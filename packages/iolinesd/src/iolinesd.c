#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "conf.h"
#include "log.h"

#define STATE_UNKNOWN	0
#define STATE_BELOW	1
#define STATE_ABOVE	2

#define FAIL_NONE	0
#define FAIL_PRECOMMAND	1
#define FAIL_READ	2

#define MODE_UNKNOWN	(-1)

#define CONTROL_TIMEOUT		5
#define CONTROL_CLIENT_TIMEOUT	1
#define MAX_REQUEST		2048
#define MAX_FIELDS		8
#define MAX_COMMAND_ID		1024

#define PULSE_MIN		1
#define PULSE_MAX		3600

#define EVENT_CLOSED	0
#define EVENT_OPENED	1
#define EVENT_MODE	2

#define MAX_EVENTS	16

struct event {
	struct event *next;
	int action;
	int mode;
	char source[8];
	char *id;
};

struct line_state {
	int64_t due;
	int64_t pulse_end;
	pid_t child;
	pid_t hotplug;
	int state;
	int failed;
	int mode;
	int prev_mode;
	int controlled;
	char pulse_source[8];
	char pulse_id[MAX_COMMAND_ID];
	struct event *events;
	int nevents;
};

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t reload_requested = 0;
static int wake_pipe[2] = { -1, -1 };

static void wake(void)
{
	int saved = errno;
	ssize_t rc = write(wake_pipe[1], "", 1);

	(void)rc;
	errno = saved;
}

static void stop_handler(int sig)
{
	(void)sig;
	running = 0;
	wake();
}

static void hup_handler(int sig)
{
	(void)sig;
	reload_requested = 1;
	wake();
}

static int wake_pipe_open(void)
{
	int i;

	if (pipe(wake_pipe))
		return -1;
	for (i = 0; i < 2; i++) {
		fcntl(wake_pipe[i], F_SETFD, FD_CLOEXEC);
		fcntl(wake_pipe[i], F_SETFL, fcntl(wake_pipe[i], F_GETFL) | O_NONBLOCK);
	}

	return 0;
}

static void wake_pipe_drain(void)
{
	char buf[64];

	while (read(wake_pipe[0], buf, sizeof(buf)) > 0)
		;
}

static int usage(void)
{
	fprintf(stderr,
		"Usage: iolinesd [-c file] [-g name] [-n] [-t] [-v]\n"
		"       iolinesd -l name -a close|open|pulse [-d sec] [-s cloud|web|local] [-i id]\n"
		"  -c file  configuration file (default " DEFAULT_CONF ")\n"
		"  -g name  print a snapshot of one line as JSON and exit\n"
		"  -n       check the configuration and exit, non-zero if there is nothing to poll\n"
		"  -t       print a snapshot of all lines as JSON and exit\n"
		"  -v       log every sample, not only threshold crossings\n"
		"  -l name  switch the output of a line through the running daemon and exit\n"
		"  -a       close (hold in oc), open (back to the base mode) or pulse\n"
		"  -d sec   pulse length, %d..%d\n"
		"  -s       who switches: cloud, web or local (default local)\n"
		"  -i id    command id of the switch\n",
		PULSE_MIN, PULSE_MAX);

	return 1;
}

static int64_t now_ms(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int read_raw(const char *path, long *raw)
{
	FILE *f;
	long v;

	f = fopen(path, "r");
	if (!f)
		return -1;
	if (fscanf(f, "%ld", &v) != 1) {
		fclose(f);
		errno = EINVAL;
		return -1;
	}
	fclose(f);
	*raw = v;

	return 0;
}

static long to_value(const struct line_conf *c, long raw)
{
	double v = raw * c->scale + c->offset;

	return (long)(v < 0 ? v - 0.5 : v + 0.5);
}

static int to_state(const struct line_conf *c, long value, int prev)
{
	if (value > c->threshold + c->hysteresis)
		return STATE_ABOVE;
	if (value < c->threshold - c->hysteresis)
		return STATE_BELOW;
	if (prev != STATE_UNKNOWN)
		return prev;

	return value >= c->threshold ? STATE_ABOVE : STATE_BELOW;
}

static const char *edge_name(int state)
{
	return state == STATE_ABOVE ? "above" : "below";
}

static const char *edge_filter_name(int edge)
{
	if (edge == EDGE_RISING)
		return "rising";
	if (edge == EDGE_FALLING)
		return "falling";

	return "both";
}

static const char *mode_name(int mode)
{
	if (mode == MODE_OC)
		return "oc";
	if (mode == MODE_DRY_CONTACT)
		return "dry_contact";

	return "adc";
}

static void set_line_env(const struct line_conf *c, int index)
{
	char buf[32];

	setenv("IO_NAME", c->name, 1);
	setenv("IO_PATH", c->source, 1);
	snprintf(buf, sizeof(buf), "%d", index);
	setenv("IO_LINE", buf, 1);
}

static void set_env(const struct line_conf *c, int index, int mode, int state,
		    long value, long raw)
{
	char buf[32];

	set_line_env(c, index);
	setenv("IO_MODE", mode_name(mode), 1);
	setenv("ACTION", edge_name(state), 1);
	setenv("IO_EDGE", edge_name(state), 1);
	setenv("IO_EDGE_FILTER", edge_filter_name(c->edge), 1);
	snprintf(buf, sizeof(buf), "%ld", value);
	setenv("IO_VALUE", buf, 1);
	snprintf(buf, sizeof(buf), "%ld", raw);
	setenv("IO_RAW", buf, 1);
	snprintf(buf, sizeof(buf), "%ld", c->threshold);
	setenv("IO_THRESHOLD", buf, 1);
	snprintf(buf, sizeof(buf), "%ld", c->hysteresis);
	setenv("IO_HYSTERESIS", buf, 1);
}

static pid_t spawn(const struct line_conf *c, int index, int mode, int state,
		   long value, long raw, int hotplug)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		ERROR("%s: fork failed: %s\n", c->name, strerror(errno));
		return -1;
	}
	if (pid > 0)
		return pid;

	set_env(c, index, mode, state, value, raw);

	if (hotplug)
		execl(HOTPLUG_CALL, "hotplug-call", HOTPLUG_SUBSYSTEM, (char *)NULL);
	else
		execl("/bin/sh", "sh", "-c", c->command, (char *)NULL);

	ERROR("%s: cannot execute %s: %s\n", c->name,
	      hotplug ? HOTPLUG_CALL : "/bin/sh", strerror(errno));
	_exit(127);
}

static int run_shell(const struct line_conf *c, int index, const char *cmd)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid < 0) {
		ERROR("%s: fork failed: %s\n", c->name, strerror(errno));
		return -1;
	}
	if (!pid) {
		set_line_env(c, index);
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		ERROR("%s: cannot execute /bin/sh: %s\n", c->name, strerror(errno));
		_exit(127);
	}

	if (waitpid(pid, &status, 0) < 0) {
		ERROR("%s: waitpid failed: %s\n", c->name, strerror(errno));
		return -1;
	}
	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	return 128 + WTERMSIG(status);
}

static int run_precommand(const struct line_conf *c, int index)
{
	if (!c->precommand[0])
		return 0;

	return run_shell(c, index, c->precommand);
}

static int base_mode(const struct line_conf *c)
{
	return c->mode == MODE_OC ? MODE_ADC : c->mode;
}

static int mode_commands(const struct line_conf *c)
{
	return !!c->oc_on[0] + !!c->oc_off[0] + !!c->dc_on[0] + !!c->dc_off[0];
}

static int run_mode(const struct line_conf *c, int index, int mode)
{
	const char *cmd[2];
	int i, rc;

	if (mode == MODE_OC) {
		cmd[0] = c->dc_off;
		cmd[1] = c->oc_on;
	} else if (mode == MODE_DRY_CONTACT) {
		cmd[0] = c->oc_off;
		cmd[1] = c->dc_on;
	} else {
		cmd[0] = c->oc_off;
		cmd[1] = c->dc_off;
	}

	for (i = 0; i < 2; i++) {
		rc = run_shell(c, index, cmd[i]);
		if (rc) {
			ERROR("%s: mode command failed with status %d: %s\n", c->name, rc, cmd[i]);
			return -1;
		}
	}

	return 0;
}

static int apply_mode(const struct line_conf *c, struct line_state *st, int index, int mode)
{
	int k;

	st->mode = mode;
	st->state = STATE_UNKNOWN;

	k = mode_commands(c);
	if (!k)
		return -1;
	if (k < 4) {
		ERROR("%s: no mode commands, mode %s not applied\n", c->name, mode_name(mode));
		return -1;
	}

	k = run_mode(c, index, mode);
	LOG("%s: mode %s\n", c->name, mode_name(mode));

	return k;
}

static int switch_mode(const struct line_conf *c, struct line_state *st, int index, int mode)
{
	if (mode_commands(c) < 4) {
		ERROR("%s: no mode commands, mode %s not applied\n", c->name, mode_name(mode));
		return -1;
	}
	if (run_mode(c, index, mode)) {
		ERROR("%s: mode %s failed, back to %s\n", c->name, mode_name(mode),
		      mode_name(st->mode));
		run_mode(c, index, st->mode);
		return -1;
	}

	st->mode = mode;
	st->state = STATE_UNKNOWN;

	return 0;
}

static const char *event_name(int action)
{
	if (action == EVENT_CLOSED)
		return "closed";
	if (action == EVENT_OPENED)
		return "opened";

	return "mode";
}

static pid_t spawn_event(const struct line_conf *c, int index, const struct event *ev)
{
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		ERROR("%s: fork failed: %s\n", c->name, strerror(errno));
		return -1;
	}
	if (pid > 0)
		return pid;

	set_line_env(c, index);
	setenv("ACTION", event_name(ev->action), 1);
	setenv("IO_MODE", mode_name(ev->mode), 1);
	setenv("IO_SOURCE", ev->source, 1);
	if (ev->id)
		setenv("IO_COMMAND_ID", ev->id, 1);

	execl(HOTPLUG_CALL, "hotplug-call", HOTPLUG_SUBSYSTEM, (char *)NULL);
	ERROR("%s: cannot execute %s: %s\n", c->name, HOTPLUG_CALL, strerror(errno));
	_exit(127);
}

static void free_events(struct event *ev)
{
	struct event *next;

	for (; ev; ev = next) {
		next = ev->next;
		free(ev->id);
		free(ev);
	}
}

static void start_events(const struct line_conf *c, struct line_state *st, int index)
{
	struct event *ev;
	pid_t pid;

	while (st->events && st->hotplug <= 0) {
		ev = st->events;
		st->events = ev->next;
		st->nevents--;
		ev->next = NULL;
		pid = spawn_event(c, index, ev);
		if (pid > 0)
			st->hotplug = pid;
		free_events(ev);
	}
}

static void queue_event(const struct line_conf *c, struct line_state *st, int index,
			int action, const char *source, const char *id)
{
	struct event *ev, **tail;

	if (!c->hotplug)
		return;
	if (st->nevents >= MAX_EVENTS) {
		ERROR("%s: %d events wait for hotplug, %s dropped\n", c->name, st->nevents,
		      event_name(action));
		return;
	}

	ev = calloc(1, sizeof(*ev));
	if (!ev) {
		ERROR("%s: out of memory, %s dropped\n", c->name, event_name(action));
		return;
	}
	ev->action = action;
	ev->mode = st->mode;
	snprintf(ev->source, sizeof(ev->source), "%s", source);
	if (id && id[0] && (!strcmp(source, "cloud") || !strcmp(source, "timer"))) {
		ev->id = strdup(id);
		if (!ev->id) {
			ERROR("%s: out of memory, %s dropped\n", c->name, event_name(action));
			free(ev);
			return;
		}
	}

	for (tail = &st->events; *tail; tail = &(*tail)->next)
		;
	*tail = ev;
	st->nevents++;

	start_events(c, st, index);
}

static void reload_event(const struct line_conf *c, struct line_state *st, int index)
{
	if (st->prev_mode == MODE_UNKNOWN || st->prev_mode == st->mode)
		return;

	if (st->prev_mode == MODE_OC)
		queue_event(c, st, index, EVENT_OPENED, "web", NULL);
	else if (st->mode == MODE_OC)
		queue_event(c, st, index, EVENT_CLOSED, "web", NULL);
	else
		queue_event(c, st, index, EVENT_MODE, "web", NULL);
}

static int valid_action(const char *action)
{
	return !strcmp(action, "close") || !strcmp(action, "open") || !strcmp(action, "pulse");
}

static int valid_source(const char *source)
{
	return !strcmp(source, "cloud") || !strcmp(source, "web") || !strcmp(source, "local");
}

static long parse_duration(const char *s)
{
	char *end;
	long v;

	if (*s < '0' || *s > '9')
		return -1;
	errno = 0;
	v = strtol(s, &end, 10);
	if (errno || *end || v < PULSE_MIN || v > PULSE_MAX)
		return -1;

	return v;
}

static const char *line_control(const struct line_conf *c, struct line_state *st, int index,
				const char *action, const char *duration,
				const char *source, const char *id)
{
	int closed = st->mode == MODE_OC;
	long sec = 0;

	if (!strcmp(action, "pulse")) {
		sec = parse_duration(duration);
		if (sec < 0)
			return "bad_duration";
	}
	if (strcmp(action, "open") && st->pulse_end)
		return "busy";

	if (!strcmp(action, "close")) {
		if (closed)
			return "no_change";
		if (switch_mode(c, st, index, MODE_OC))
			return "failed";
		st->controlled = 1;
		queue_event(c, st, index, EVENT_CLOSED, source, id);
		return "ok";
	}

	if (!strcmp(action, "pulse")) {
		if (!closed) {
			if (switch_mode(c, st, index, MODE_OC))
				return "failed";
			queue_event(c, st, index, EVENT_CLOSED, source, id);
		}
		st->controlled = 1;
		st->pulse_end = now_ms() + sec * 1000;
		snprintf(st->pulse_source, sizeof(st->pulse_source), "%s", source);
		snprintf(st->pulse_id, sizeof(st->pulse_id), "%s", id);
		return closed ? "already_closed" : "ok";
	}

	if (!closed)
		return "no_change";
	if (switch_mode(c, st, index, base_mode(c)))
		return "failed";
	st->controlled = 1;
	st->pulse_end = 0;
	queue_event(c, st, index, EVENT_OPENED, source, id);

	return "ok";
}

static void end_pulse(const struct line_conf *c, struct line_state *st, int index)
{
	st->pulse_end = 0;

	if (switch_mode(c, st, index, base_mode(c))) {
		ERROR("%s: pulse ended, the line stays closed\n", c->name);
		return;
	}

	LOG("%s: pulse ended, mode %s\n", c->name, mode_name(st->mode));
	if (!strcmp(st->pulse_source, "cloud"))
		queue_event(c, st, index, EVENT_OPENED, "timer", st->pulse_id);
	else
		queue_event(c, st, index, EVENT_OPENED, "local", NULL);
}

static void run_command(const struct line_conf *c, struct line_state *st,
			int index, int state, long value, long raw)
{
	pid_t pid;

	if (!c->command[0])
		return;

	if (!(c->edge & (state == STATE_ABOVE ? EDGE_RISING : EDGE_FALLING)))
		return;

	if (st->child > 0) {
		ERROR("%s: previous command still running, not started\n", c->name);
		return;
	}

	pid = spawn(c, index, st->mode, state, value, raw, 0);
	if (pid > 0)
		st->child = pid;
}

static void run_hotplug(const struct line_conf *c, struct line_state *st,
			int index, int state, long value, long raw)
{
	pid_t pid;

	if (!c->hotplug)
		return;

	if (st->hotplug > 0) {
		ERROR("%s: previous hotplug handler still running, not started\n", c->name);
		return;
	}

	pid = spawn(c, index, st->mode, state, value, raw, 1);
	if (pid > 0)
		st->hotplug = pid;
}

static void reap(struct line_state *state, int n)
{
	int status;
	pid_t pid;
	int i;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
		for (i = 0; i < n; i++) {
			if (state[i].child == pid)
				state[i].child = 0;
			if (state[i].hotplug == pid)
				state[i].hotplug = 0;
		}
}

static void print_line(FILE *f, const struct line_conf *c, const struct line_state *st,
		       int index, const char *sep)
{
	char mode[96] = "";
	int64_t left;
	long raw, value;
	int state;

	if (st) {
		snprintf(mode, sizeof(mode), ", \"mode\": \"%s\", \"base\": \"%s\"",
			 mode_name(st->mode), mode_name(base_mode(c)));
		if (st->pulse_end) {
			left = st->pulse_end - now_ms();
			snprintf(mode + strlen(mode), sizeof(mode) - strlen(mode), ", \"pulse\": %ld",
				 left > 0 ? (long)((left + 999) / 1000) : 0L);
		}
	}

	if (run_precommand(c, index)) {
		fprintf(f, "\t\"%s\": { \"error\": \"precommand failed\"%s }%s\n",
			c->name, mode, sep);
		return;
	}
	if (read_raw(c->source, &raw)) {
		fprintf(f, "\t\"%s\": { \"error\": \"%s\"%s }%s\n",
			c->name, strerror(errno), mode, sep);
		return;
	}
	value = to_value(c, raw);
	if (!c->has_threshold) {
		fprintf(f, "\t\"%s\": { \"raw\": %ld, \"value\": %ld%s }%s\n",
			c->name, raw, value, mode, sep);
		return;
	}
	fprintf(f, "\t\"%s\": { \"raw\": %ld, \"value\": %ld%s, \"threshold\": %ld, \"hysteresis\": %ld, \"edge\": \"%s\"",
		c->name, raw, value, mode, c->threshold, c->hysteresis, edge_filter_name(c->edge));
	if (!st || st->mode != MODE_OC) {
		state = st && st->state != STATE_UNKNOWN ? st->state :
			to_state(c, value, STATE_UNKNOWN);
		fprintf(f, ", \"state\": \"%s\"", edge_name(state));
	}
	fprintf(f, " }%s\n", sep);
}

static int find_line(const struct line_conf *lines, int n, const char *name)
{
	int i;

	for (i = 0; i < n; i++)
		if (!strcmp(lines[i].name, name))
			return i;

	return -1;
}

static void snapshot(FILE *f, const struct line_conf *lines, const struct line_state *state,
		     int n, int only)
{
	int i;

	fprintf(f, "{\n");
	if (only >= 0)
		print_line(f, &lines[only], state ? &state[only] : NULL, only, "");
	else
		for (i = 0; i < n; i++)
			print_line(f, &lines[i], state ? &state[i] : NULL, i, i + 1 < n ? "," : "");
	fprintf(f, "}\n");
}

static int local_snapshot(const struct line_conf *lines, int n, const char *name)
{
	int i = -1;

	if (name) {
		i = find_line(lines, n, name);
		if (i < 0) {
			ERROR("%s: no such line\n", name);
			return 1;
		}
	}
	snapshot(stdout, lines, NULL, n, i);

	return 0;
}

static void set_timeouts(int fd, int sec)
{
	struct timeval tv = { .tv_sec = sec };

	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static void control_address(struct sockaddr_un *addr)
{
	memset(addr, 0, sizeof(*addr));
	addr->sun_family = AF_UNIX;
	snprintf(addr->sun_path, sizeof(addr->sun_path), "%s", CONTROL_SOCKET);
}

static int control_open(void)
{
	struct sockaddr_un addr;
	mode_t mask;
	int fd, rc;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

	control_address(&addr);
	unlink(CONTROL_SOCKET);
	mask = umask(077);
	rc = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	umask(mask);
	if (rc || listen(fd, 4)) {
		rc = errno;
		close(fd);
		errno = rc;
		return -1;
	}

	return fd;
}

static int control_connect(void)
{
	struct sockaddr_un addr;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	control_address(&addr);
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
		close(fd);
		return -1;
	}
	set_timeouts(fd, CONTROL_TIMEOUT);

	return fd;
}

static int control_send(int fd, const char *const *field, int count)
{
	char buf[MAX_REQUEST];
	size_t len = 0, l;
	ssize_t w;
	int i;

	for (i = 0; i < count; i++) {
		l = strlen(field[i]) + 1;
		if (len + l > sizeof(buf)) {
			errno = EMSGSIZE;
			return -1;
		}
		memcpy(buf + len, field[i], l);
		len += l;
	}
	for (l = 0; l < len; l += w) {
		w = write(fd, buf + l, len - l);
		if (w < 0)
			return -1;
	}

	return shutdown(fd, SHUT_WR);
}

static int query_daemon(const char *name)
{
	const char *field[2] = { "get", name };
	char buf[1024];
	size_t len;
	FILE *f;
	int fd;

	fd = control_connect();
	if (fd < 0)
		return -1;

	if (control_send(fd, field, name ? 2 : 1)) {
		ERROR("cannot send a request to iolinesd: %s\n", strerror(errno));
		close(fd);
		return 1;
	}

	f = fdopen(fd, "r");
	if (!f) {
		ERROR("cannot read the answer of iolinesd: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	if (!fgets(buf, sizeof(buf), f)) {
		ERROR("no answer from iolinesd\n");
		fclose(f);
		return 1;
	}
	if (strcmp(buf, "ok\n")) {
		if (!strcmp(buf, "no_line\n"))
			ERROR("%s: no such line\n", name);
		else
			ERROR("unexpected answer from iolinesd: %s", buf);
		fclose(f);
		return 1;
	}
	while ((len = fread(buf, 1, sizeof(buf), f)) > 0)
		fwrite(buf, 1, len, stdout);
	if (ferror(f)) {
		ERROR("answer of iolinesd interrupted\n");
		fclose(f);
		return 1;
	}
	fclose(f);

	return 0;
}

static int control_line(const char *name, const char *action, const char *duration,
			const char *source, const char *id)
{
	const char *field[6] = { "set", name, action, duration, source, id };
	char buf[64];
	FILE *f;
	int fd;

	if (!valid_action(action)) {
		ERROR("unknown action '%s'\n", action);
		return 2;
	}
	if (!valid_source(source)) {
		ERROR("unknown source '%s'\n", source);
		return 2;
	}
	if (strlen(id) >= MAX_COMMAND_ID) {
		ERROR("command id longer than %d bytes\n", MAX_COMMAND_ID - 1);
		return 2;
	}

	fd = control_connect();
	if (fd < 0) {
		ERROR("cannot connect to iolinesd: %s\n", strerror(errno));
		return 2;
	}
	if (control_send(fd, field, 6)) {
		ERROR("cannot send a request to iolinesd: %s\n", strerror(errno));
		close(fd);
		return 2;
	}

	f = fdopen(fd, "r");
	if (!f) {
		ERROR("cannot read the answer of iolinesd: %s\n", strerror(errno));
		close(fd);
		return 2;
	}
	if (!fgets(buf, sizeof(buf), f)) {
		ERROR("no answer from iolinesd\n");
		fclose(f);
		return 2;
	}
	fclose(f);

	if (!strcmp(buf, "ok\n") || !strcmp(buf, "no_change\n") ||
	    !strcmp(buf, "already_closed\n")) {
		fputs(buf, stdout);
		return 0;
	}
	if (!strcmp(buf, "no_line\n") || !strcmp(buf, "bad_duration\n") ||
	    !strcmp(buf, "busy\n") || !strcmp(buf, "failed\n")) {
		fputs(buf, stdout);
		return 1;
	}

	ERROR("unexpected answer from iolinesd: %s", buf);

	return 2;
}

static void control_answer(FILE *f, const char **field, int nf,
			   const struct line_conf *lines, struct line_state *state, int n)
{
	const char *result;
	int i = -1;

	if (nf == 6 && !strcmp(field[0], "set")) {
		if (!valid_action(field[2]) || !valid_source(field[4]) ||
		    strlen(field[5]) >= MAX_COMMAND_ID) {
			fputs("bad_request\n", f);
			return;
		}
		i = find_line(lines, n, field[1]);
		result = i < 0 ? "no_line" :
			line_control(&lines[i], &state[i], i, field[2], field[3], field[4], field[5]);
		fprintf(f, "%s\n", result);
		LOG("%s: %s%s%s by %s%s%s: %s\n", field[1], field[2],
		    strcmp(field[2], "pulse") ? "" : " ", strcmp(field[2], "pulse") ? "" : field[3],
		    field[4], field[5][0] ? ", command " : "", field[5], result);
		return;
	}

	if (nf >= 1 && nf <= 2 && !strcmp(field[0], "get")) {
		if (nf == 2) {
			i = find_line(lines, n, field[1]);
			if (i < 0) {
				fputs("no_line\n", f);
				return;
			}
		}
		fputs("ok\n", f);
		snapshot(f, lines, state, n, i);
		return;
	}

	fputs("bad_request\n", f);
}

static void control_serve(int lfd, const struct line_conf *lines,
			  struct line_state *state, int n)
{
	char req[MAX_REQUEST];
	const char *field[MAX_FIELDS];
	size_t len = 0, pos;
	ssize_t r = 0;
	int fd, nf = 0;
	FILE *f;

	fd = accept(lfd, NULL, NULL);
	if (fd < 0)
		return;
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	set_timeouts(fd, CONTROL_CLIENT_TIMEOUT);

	while (len < sizeof(req) && (r = read(fd, req + len, sizeof(req) - len)) > 0)
		len += r;
	if (r < 0) {
		ERROR("control: no request from the client: %s\n", strerror(errno));
		close(fd);
		return;
	}

	f = fdopen(fd, "w");
	if (!f) {
		close(fd);
		return;
	}

	if (len == sizeof(req) || (len && req[len - 1])) {
		fputs("bad_request\n", f);
		fclose(f);
		return;
	}
	for (pos = 0; pos < len && nf < MAX_FIELDS; pos += strlen(req + pos) + 1)
		field[nf++] = req + pos;
	if (pos < len)
		nf = 0;

	control_answer(f, field, nf, lines, state, n);
	fclose(f);
}

static int collect_polled(const struct line_conf *lines, int n, int *polled)
{
	int i, np = 0;

	for (i = 0; i < n; i++)
		if (lines[i].poll)
			polled[np++] = i;

	return np;
}

static int same_comparator(const struct line_conf *a, const struct line_conf *b)
{
	return !strcmp(a->source, b->source) && a->scale == b->scale &&
	       a->offset == b->offset && a->poll == b->poll &&
	       a->has_threshold == b->has_threshold && a->threshold == b->threshold &&
	       a->hysteresis == b->hysteresis;
}

static int same_mode_commands(const struct line_conf *a, const struct line_conf *b)
{
	return !strcmp(a->oc_on, b->oc_on) && !strcmp(a->oc_off, b->oc_off) &&
	       !strcmp(a->dc_on, b->dc_on) && !strcmp(a->dc_off, b->dc_off);
}

static int reload(const char *path, struct line_conf *lines, struct line_state *state, int n)
{
	static struct line_conf fresh[MAX_LINES];
	static struct line_state kept[MAX_LINES];
	int m, i, j;

	m = conf_load(path, fresh, MAX_LINES);
	if (m < 0) {
		ERROR("reload failed, previous configuration kept\n");
		return -1;
	}

	memset(kept, 0, sizeof(kept));
	for (i = 0; i < m; i++) {
		kept[i].mode = MODE_UNKNOWN;
		kept[i].prev_mode = MODE_UNKNOWN;
		for (j = 0; j < n; j++)
			if (!strcmp(fresh[i].name, lines[j].name))
				break;
		if (j == n)
			continue;
		kept[i].child = state[j].child;
		kept[i].hotplug = state[j].hotplug;
		kept[i].prev_mode = state[j].mode;
		kept[i].events = state[j].events;
		kept[i].nevents = state[j].nevents;
		state[j].events = NULL;
		if (same_mode_commands(&fresh[i], &lines[j])) {
			if (state[j].controlled && fresh[i].mode == lines[j].mode) {
				kept[i].mode = state[j].mode;
				kept[i].controlled = 1;
				kept[i].pulse_end = state[j].pulse_end;
				memcpy(kept[i].pulse_source, state[j].pulse_source,
				       sizeof(kept[i].pulse_source));
				memcpy(kept[i].pulse_id, state[j].pulse_id, sizeof(kept[i].pulse_id));
			} else if (!state[j].controlled && state[j].mode == fresh[i].mode)
				kept[i].mode = state[j].mode;
		}
		if (same_comparator(&fresh[i], &lines[j])) {
			kept[i].due = state[j].due;
			kept[i].state = state[j].state;
			kept[i].failed = state[j].failed;
		}
	}

	for (j = 0; j < n; j++)
		free_events(state[j].events);

	memcpy(lines, fresh, sizeof(*lines) * m);
	memcpy(state, kept, sizeof(*state) * m);

	return m;
}

int main(int argc, char **argv)
{
	struct line_conf lines[MAX_LINES];
	struct line_state state[MAX_LINES];
	int polled[MAX_LINES];
	struct pollfd pfd[2];
	const char *conf = DEFAULT_CONF;
	const char *get = NULL;
	const char *line = NULL;
	const char *action = NULL;
	const char *duration = "";
	const char *source = "local";
	const char *id = "";
	int check = 0;
	int test = 0;
	int verbose = 0;
	int n, np, i, k, ch, boot, control;

	while ((ch = getopt(argc, argv, "c:g:ntvl:a:d:s:i:h")) != -1) {
		switch (ch) {
		case 'c':
			conf = optarg;
			break;
		case 'g':
			get = optarg;
			break;
		case 'n':
			check = 1;
			break;
		case 't':
			test = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'l':
			line = optarg;
			break;
		case 'a':
			action = optarg;
			break;
		case 'd':
			duration = optarg;
			break;
		case 's':
			source = optarg;
			break;
		case 'i':
			id = optarg;
			break;
		default:
			return usage();
		}
	}

	if (line || action) {
		if (!line || !action)
			return usage();
		return control_line(line, action, duration, source, id);
	}

	if (test || get) {
		i = query_daemon(get);
		if (i >= 0)
			return i;
	}

	n = conf_load(conf, lines, MAX_LINES);
	if (n < 0)
		return 1;

	if (test || get)
		return local_snapshot(lines, n, get);

	np = collect_polled(lines, n, polled);

	if (check) {
		if (!np)
			LOG("nothing to do, no monitored lines in %s\n", conf);
		return np ? 0 : 1;
	}

	if (wake_pipe_open()) {
		ERROR("cannot create a pipe: %s\n", strerror(errno));
		return 1;
	}

	signal(SIGTERM, stop_handler);
	signal(SIGINT, stop_handler);
	signal(SIGHUP, hup_handler);
	signal(SIGPIPE, SIG_IGN);

	memset(state, 0, sizeof(state));

	boot = access(BOOT_MARKER, F_OK) != 0;
	for (i = 0; i < n; i++)
		apply_mode(&lines[i], &state[i], i,
			   boot && !lines[i].enabled ? MODE_ADC : lines[i].mode);
	if (boot) {
		FILE *f = fopen(BOOT_MARKER, "w");

		if (f)
			fclose(f);
		else
			ERROR("cannot create %s: %s\n", BOOT_MARKER, strerror(errno));
	}

	control = control_open();
	if (control < 0)
		ERROR("cannot open %s: %s\n", CONTROL_SOCKET, strerror(errno));

	LOG("started%s, %d monitored line(s) from %s\n", boot ? " after boot" : "", np, conf);

	while (running) {
		int64_t now;
		int64_t next = 0;
		int have_next = 0;
		int timeout;

		if (reload_requested) {
			reload_requested = 0;
			i = reload(conf, lines, state, n);
			if (i >= 0) {
				n = i;
				np = collect_polled(lines, n, polled);
				for (i = 0; i < n; i++)
					if (state[i].mode == MODE_UNKNOWN &&
					    !apply_mode(&lines[i], &state[i], i, lines[i].mode))
						reload_event(&lines[i], &state[i], i);
				LOG("reloaded, %d monitored line(s) from %s\n", np, conf);
			}
		}

		now = now_ms();
		reap(state, n);

		for (i = 0; i < n; i++)
			start_events(&lines[i], &state[i], i);

		for (i = 0; i < n; i++)
			if (state[i].pulse_end && state[i].pulse_end <= now)
				end_pulse(&lines[i], &state[i], i);

		for (k = 0; k < np; k++) {
			long raw, value;
			int st, rc;

			i = polled[k];

			if (state[i].mode == MODE_OC)
				continue;
			if (state[i].due > now)
				continue;

			state[i].due = now + lines[i].interval;

			rc = run_precommand(&lines[i], i);
			if (rc) {
				if (state[i].failed != FAIL_PRECOMMAND)
					ERROR("%s: precommand failed with status %d\n",
					      lines[i].name, rc);
				state[i].failed = FAIL_PRECOMMAND;
				continue;
			}
			if (read_raw(lines[i].source, &raw)) {
				if (state[i].failed != FAIL_READ)
					ERROR("%s: cannot read %s: %s\n", lines[i].name,
					      lines[i].source, strerror(errno));
				state[i].failed = FAIL_READ;
				continue;
			}
			if (state[i].failed) {
				LOG("%s: %s is readable again\n", lines[i].name, lines[i].source);
				state[i].failed = FAIL_NONE;
			}

			value = to_value(&lines[i], raw);
			st = to_state(&lines[i], value, state[i].state);

			if (verbose)
				LOG("%s: raw %ld, value %ld, threshold %ld, hysteresis %ld, %s\n",
				    lines[i].name, raw, value, lines[i].threshold,
				    lines[i].hysteresis, edge_name(st));

			if (state[i].state == STATE_UNKNOWN) {
				state[i].state = st;
				continue;
			}
			if (state[i].state == st)
				continue;

			state[i].state = st;
			LOG("%s: %s threshold %ld, hysteresis %ld, value %ld\n", lines[i].name,
			    edge_name(st), lines[i].threshold, lines[i].hysteresis, value);
			run_command(&lines[i], &state[i], i, st, value, raw);
			run_hotplug(&lines[i], &state[i], i, st, value, raw);
		}

		for (k = 0; k < np; k++) {
			i = polled[k];
			if (state[i].mode == MODE_OC)
				continue;
			if (!have_next || state[i].due < next) {
				next = state[i].due;
				have_next = 1;
			}
		}
		for (i = 0; i < n; i++)
			if (state[i].pulse_end && (!have_next || state[i].pulse_end < next)) {
				next = state[i].pulse_end;
				have_next = 1;
			}

		now = now_ms();
		timeout = -1;
		if (have_next)
			timeout = next > now ? (int)(next - now) : 0;

		pfd[0].fd = wake_pipe[0];
		pfd[0].events = POLLIN;
		pfd[1].fd = control;
		pfd[1].events = POLLIN;
		if (poll(pfd, 2, timeout) > 0) {
			if (pfd[0].revents & POLLIN)
				wake_pipe_drain();
			if (pfd[1].revents & POLLIN)
				control_serve(control, lines, state, n);
		}
	}

	if (control >= 0) {
		close(control);
		unlink(CONTROL_SOCKET);
	}

	LOG("stopped\n");

	return 0;
}

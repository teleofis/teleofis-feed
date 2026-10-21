#ifndef _conf_h
#define _conf_h

#define MAX_LINES	32
#define MAX_NAME	64
#define MAX_SOURCE	256
#define MAX_COMMAND	512

#define DEFAULT_CONF		"/etc/iolinesd.conf"
#define DEFAULT_INTERVAL	1000

#ifndef HOTPLUG_CALL
#define HOTPLUG_CALL		"/sbin/hotplug-call"
#endif

#ifndef BOOT_MARKER
#define BOOT_MARKER		"/var/run/iolinesd.boot"
#endif

#ifndef CONTROL_SOCKET
#define CONTROL_SOCKET		"/var/run/iolinesd.sock"
#endif

#define HOTPLUG_SUBSYSTEM	"iolines"

#define EDGE_RISING	1
#define EDGE_FALLING	2
#define EDGE_BOTH	(EDGE_RISING | EDGE_FALLING)

#define MODE_ADC		0
#define MODE_DRY_CONTACT	1
#define MODE_OC			2

struct line_conf {
	char name[MAX_NAME];
	char source[MAX_SOURCE];
	char command[MAX_COMMAND];
	char precommand[MAX_COMMAND];
	char oc_on[MAX_COMMAND];
	char oc_off[MAX_COMMAND];
	char dc_on[MAX_COMMAND];
	char dc_off[MAX_COMMAND];
	double scale;
	double offset;
	long threshold;
	long hysteresis;
	int interval;
	int poll;
	int hotplug;
	int edge;
	int has_threshold;
	int mode;
	int enabled;
};

int conf_load(const char *path, struct line_conf *lines, int max);

#endif

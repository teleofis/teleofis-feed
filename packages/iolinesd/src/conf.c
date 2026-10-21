#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "log.h"

static char *trim(char *s)
{
	char *end;

	while (isspace((unsigned char)*s))
		s++;
	if (!*s)
		return s;
	end = s + strlen(s) - 1;
	while (end > s && isspace((unsigned char)*end))
		*end-- = '\0';

	return s;
}

static void set_option(struct line_conf *c, const char *key, const char *val,
		       const char *path, int lineno)
{
	if (!strcmp(key, "source"))
		snprintf(c->source, sizeof(c->source), "%s", val);
	else if (!strcmp(key, "command"))
		snprintf(c->command, sizeof(c->command), "%s", val);
	else if (!strcmp(key, "precommand"))
		snprintf(c->precommand, sizeof(c->precommand), "%s", val);
	else if (!strcmp(key, "oc_on"))
		snprintf(c->oc_on, sizeof(c->oc_on), "%s", val);
	else if (!strcmp(key, "oc_off"))
		snprintf(c->oc_off, sizeof(c->oc_off), "%s", val);
	else if (!strcmp(key, "dc_on"))
		snprintf(c->dc_on, sizeof(c->dc_on), "%s", val);
	else if (!strcmp(key, "dc_off"))
		snprintf(c->dc_off, sizeof(c->dc_off), "%s", val);
	else if (!strcmp(key, "enabled"))
		c->enabled = atoi(val);
	else if (!strcmp(key, "mode")) {
		if (!strcmp(val, "adc"))
			c->mode = MODE_ADC;
		else if (!strcmp(val, "dry_contact"))
			c->mode = MODE_DRY_CONTACT;
		else if (!strcmp(val, "oc"))
			c->mode = MODE_OC;
		else
			ERROR("%s:%d: unknown mode '%s', ignored\n", path, lineno, val);
	}
	else if (!strcmp(key, "scale"))
		c->scale = atof(val);
	else if (!strcmp(key, "offset"))
		c->offset = atof(val);
	else if (!strcmp(key, "interval"))
		c->interval = atoi(val);
	else if (!strcmp(key, "poll"))
		c->poll = atoi(val);
	else if (!strcmp(key, "hotplug"))
		c->hotplug = atoi(val);
	else if (!strcmp(key, "threshold")) {
		c->threshold = strtol(val, NULL, 10);
		c->has_threshold = 1;
	} else if (!strcmp(key, "hysteresis"))
		c->hysteresis = strtol(val, NULL, 10);
	else if (!strcmp(key, "edge")) {
		if (!strcmp(val, "both"))
			c->edge = EDGE_BOTH;
		else if (!strcmp(val, "rising"))
			c->edge = EDGE_RISING;
		else if (!strcmp(val, "falling"))
			c->edge = EDGE_FALLING;
		else
			ERROR("%s:%d: unknown edge '%s', ignored\n", path, lineno, val);
	} else
		ERROR("%s:%d: unknown option '%s'\n", path, lineno, key);
}

int conf_load(const char *path, struct line_conf *lines, int max)
{
	struct line_conf defaults;
	struct line_conf *cur = NULL;
	char buf[1024];
	int count = 0;
	int lineno = 0;
	int i, n;
	FILE *f;

	f = fopen(path, "r");
	if (!f) {
		ERROR("cannot open %s: %s\n", path, strerror(errno));
		return -1;
	}

	memset(&defaults, 0, sizeof(defaults));
	defaults.scale = 1.0;
	defaults.interval = DEFAULT_INTERVAL;
	defaults.hotplug = 1;
	defaults.edge = EDGE_BOTH;

	while (fgets(buf, sizeof(buf), f)) {
		char *s, *key, *val, *close;

		lineno++;
		s = trim(buf);
		if (!*s || *s == '#' || *s == ';')
			continue;

		if (*s == '[') {
			close = strchr(s, ']');
			if (!close) {
				ERROR("%s:%d: unterminated section name\n", path, lineno);
				cur = NULL;
				continue;
			}
			*close = '\0';
			s = trim(s + 1);

			if (!strcmp(s, "global")) {
				cur = &defaults;
				continue;
			}
			if (count >= max) {
				ERROR("%s:%d: more than %d lines, '%s' ignored\n",
				      path, lineno, max, s);
				cur = NULL;
				continue;
			}
			cur = &lines[count++];
			*cur = defaults;
			snprintf(cur->name, sizeof(cur->name), "%s", s);
			continue;
		}

		val = strchr(s, '=');
		if (!val) {
			ERROR("%s:%d: not an option\n", path, lineno);
			continue;
		}
		*val++ = '\0';
		key = trim(s);
		val = trim(val);

		if (!cur) {
			ERROR("%s:%d: option '%s' outside of a section\n", path, lineno, key);
			continue;
		}
		set_option(cur, key, val, path, lineno);
	}

	fclose(f);

	for (i = 0, n = 0; i < count; i++) {
		if (!lines[i].source[0]) {
			ERROR("%s: no source, line skipped\n", lines[i].name);
			continue;
		}
		if (lines[i].poll && !lines[i].has_threshold) {
			ERROR("%s: no threshold, not monitored\n", lines[i].name);
			lines[i].poll = 0;
		}
		if (lines[i].hysteresis < 0) {
			ERROR("%s: negative hysteresis, ignored\n", lines[i].name);
			lines[i].hysteresis = 0;
		}
		if (lines[i].interval <= 0)
			lines[i].interval = DEFAULT_INTERVAL;
		if (n != i)
			lines[n] = lines[i];
		n++;
	}

	return n;
}

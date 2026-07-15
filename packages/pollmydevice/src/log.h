#ifndef _log_h
#define _log_h

#include <stdio.h>
#include <syslog.h>

const char *LogTag(void);

#define LOG(fmt, ...) do { \
		syslog(LOG_NOTICE,"pollmydevice: %s" fmt, LogTag(), ## __VA_ARGS__); \
		fprintf(stderr, "pollmydevice: %s" fmt, LogTag(), ## __VA_ARGS__); \
	} while (0)

#define ERROR(fmt, ...) do { \
		syslog(LOG_ERR,"pollmydevice: %s" fmt, LogTag(), ## __VA_ARGS__); \
		fprintf(stderr, "pollmydevice: %s" fmt, LogTag(), ## __VA_ARGS__); \
	} while (0)

#endif


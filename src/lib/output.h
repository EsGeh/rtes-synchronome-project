#include "global.h"

// C POSIX library headers:
// #include <cinttypes>
#include <sys/syslog.h>
#include <syslog.h> // <- write to syslog
 
#include <stdio.h> // <- work with streams


/***********************
 * Global Macros
 ***********************/

// Infos to stderr/syslog:
/*
#define INFO(STR) \
	INFO_FMT("%s",STR)
#define INFO_FMT(FMT_STR, ...) { \
	PRINT_INFO_FMT( FMT_STR, ## __VA_ARGS__ ); \
	LOG_INFO_FMT( FMT_STR, ## __VA_ARGS__ ); \
}

#define PRINT_INFO(STR) \
	PRINT_INFO_FMT("%s",STR)
#define LOG_INFO(STR) \
	LOG_INFO_FMT("%s",STR)

#define PRINT_INFO_FMT(FMT_STR, ...) \
	_PRINT_RAW( stdout, LOG_INFO, FMT_STR, ## __VA_ARGS__ )
#define LOG_INFO_FMT(FMT_STR, ...) \
	_LOG_RAW( LOG_INFO, FMT_STR, ## __VA_ARGS__ )

// ERRORS to stderr/syslog:
#define ERROR(MSG) { \
	PRINT_ERROR(MSG); \
	LOG_ERROR(MSG); \
}
#define PRINT_ERROR(MSG) \
	PRINT_ERROR_FMT("%s",MSG)
#define LOG_ERROR(MSG) \
	LOG_ERROR_FMT("%s",MSG)

// formatted ERRORS to stderr/syslog:
#define ERROR_FMT(FMT_STR, ...) { \
	PRINT_ERROR_FMT(FMT_STR, ## __VA_ARGS__ ); \
	LOG_ERROR_FMT(FMT_STR, ## __VA_ARGS__ ); \
}
#define PRINT_ERROR_FMT(FMT_STR, ...) \
	_PRINT_RAW(stderr, LOG_ERR, "ERROR: " FMT_STR, ## __VA_ARGS__ )
#define LOG_ERROR_FMT(FMT_STR, ...) \
	_LOG_RAW(LOG_ERR, FMT_STR, ## __VA_ARGS__ )

// (Secret helpers:)
#define _PRINT_RAW(DEST, PRIO, FMT_STR, ...) \
	fprintf( DEST, FMT_STR, ## __VA_ARGS__ )

#define _LOG_RAW(PRIO, FMT_STR, ...) \
	syslog(PRIO, FMT_STR, ## __VA_ARGS__ )
*/

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,

		bool info_enable_print,
		bool info_enable_log,

		bool error_enable_print,
		bool error_enable_log
);
void log_exit();

void log_info(
		char* fmt,
		...
);

void log_error(
		char* fmt,
		...
);

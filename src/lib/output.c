#include "output.h"

#include <stdarg.h>
#include <syslog.h> // <- write to syslog
#include <stdio.h>

static bool g_verbose_enable_print = false;
static bool g_verbose_enable_log = false;

static bool g_info_enable_print = true;
static bool g_info_enable_log = false;

static bool g_error_enable_print = true;
static bool g_error_enable_log = true;

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,

		bool verbose_enable_print,
		bool verbose_enable_log,

		bool info_enable_print,
		bool info_enable_log,

		bool error_enable_print,
		bool error_enable_log

)
{
	openlog(
			prefix,
			LOG_CONS,
			LOG_USER
	);
	g_verbose_enable_print = verbose_enable_print;
	g_verbose_enable_log = verbose_enable_log;
	g_info_enable_print = info_enable_print;
	g_info_enable_log = info_enable_log;
	g_error_enable_print = error_enable_print;
	g_error_enable_log = error_enable_log;
}

void log_exit(void)
{
	closelog();
}

void log_verbose(
		char* fmt,
		...
)
{
	if( g_verbose_enable_print ) {
		va_list args;
		va_start( args, fmt );
		vfprintf( stdout, fmt, args );
		va_end( args );
	}
	if( g_verbose_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_INFO, fmt, args );
		va_end( args );
	}
}

void log_info(
		char* fmt,
		...
)
{
	if( g_info_enable_print ) {
		va_list args;
		va_start( args, fmt );
		vfprintf( stdout, fmt, args );
		va_end( args );
	}
	if( g_info_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_INFO, fmt, args );
		va_end( args );
	}
}

void log_error(
		char* fmt,
		...
)
{
	if( g_error_enable_print ) {
		va_list args;
		va_start( args, fmt );
		fprintf( stderr, "ERROR: " );
		vfprintf( stderr, fmt, args );
		va_end( args );
	}
	if( g_error_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_ERR, fmt, args );
		va_end( args );
	}
}

#include "output.h"

#include <stdarg.h>
#include <syslog.h> // <- write to syslog
#include <stdio.h>

static log_config_t g_config = {

	.time_enable_print = false,
	.time_enable_log = false,

	.verbose_enable_print = false,
	.verbose_enable_log = false,

	.info_enable_print = true,
	.info_enable_log = true,

	.warning_enable_print = true,
	.warning_enable_log = true,

	.error_enable_print = true,
	.error_enable_log = true,

};

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,
		const log_config_t config
)
{
	openlog(
			prefix,
			LOG_CONS,
			LOG_USER
	);
	g_config = config;
}

void log_exit(void)
{
	closelog();
}

void log_time(
		char* fmt,
		...
)
{
	if( g_config.time_enable_print ) {
		va_list args;
		va_start( args, fmt );
		vfprintf( stdout, fmt, args );
		va_end( args );
	}
	if( g_config.time_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_INFO, fmt, args );
		va_end( args );
	}
}

void log_verbose(
		char* fmt,
		...
)
{
	if( g_config.verbose_enable_print ) {
		va_list args;
		va_start( args, fmt );
		vfprintf( stdout, fmt, args );
		va_end( args );
	}
	if( g_config.verbose_enable_log ) {
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
	if( g_config.info_enable_print ) {
		va_list args;
		va_start( args, fmt );
		vfprintf( stdout, fmt, args );
		va_end( args );
	}
	if( g_config.info_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_INFO, fmt, args );
		va_end( args );
	}
}

void log_warning(
		char* fmt,
		...
)
{
	if( g_config.warning_enable_print ) {
		va_list args;
		va_start( args, fmt );
		fprintf( stderr, "WARNING: " );
		vfprintf( stderr, fmt, args );
		va_end( args );
	}
	if( g_config.warning_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_WARNING, fmt, args );
		va_end( args );
	}
}

void log_error(
		char* fmt,
		...
)
{
	if( g_config.error_enable_print ) {
		va_list args;
		va_start( args, fmt );
		fprintf( stderr, "ERROR: " );
		vfprintf( stderr, fmt, args );
		va_end( args );
	}
	if( g_config.error_enable_log ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( LOG_ERR, fmt, args );
		va_end( args );
	}
}

#include "output.h"

#include "global.h"
#include "lib/spsc_queue.h"

#include <stdarg.h>
#include <sys/syslog.h>
#include <syslog.h> // <- write to syslog
#include <stdio.h>
#include <pthread.h>
#include <threads.h>
#include <time.h>
#include <string.h>


/***********************
 * Types
 ***********************/

typedef struct {
	int level;
	char msg[STR_BUFFER_SIZE];
} log_entry_t;


#define LOG_QUEUE_COUNT 16384
#define DB_LOG(fmt,...)
#define ERR_LOG(fmt,...) { \
	fprintf( stderr, fmt, ## __VA_ARGS__ ); \
	exit(EXIT_FAILURE); \
}
DECL_SPSC_QUEUE(log_queue,log_entry_t)

/***********************
 * Global Data
 ***********************/

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

static _Atomic bool threaded_log = false;
static log_queue_t log_queue;
static pthread_mutex_t queue_mutex;

/***********************
 * Function Declarations
 ***********************/

void log_init(
		const char* prefix,
		const log_config_t config
)
{
	pthread_mutex_init( &queue_mutex, 0);
	log_queue_init( &log_queue, LOG_QUEUE_COUNT );
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
	log_queue_exit( &log_queue );
	pthread_mutex_destroy( &queue_mutex );
}

void log_time(
		const char* fmt,
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
		if( !threaded_log ) {
			va_list args;
			va_start( args, fmt );
			vsyslog( LOG_INFO, fmt, args );
			va_end( args );
		}
		else {
			va_list args;
			va_start( args, fmt );
			pthread_mutex_lock( &queue_mutex );
			log_entry_t* entry = NULL;
			log_queue_push_start( &log_queue, &entry );
			vsprintf( entry->msg, fmt, args );
			entry->level = LOG_INFO;
			log_queue_push_end( &log_queue );
			pthread_mutex_unlock( &queue_mutex );
			va_end( args );
		}
	}
}

void log_verbose(
		const char* fmt,
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
		if( !threaded_log ) {
			va_list args;
			va_start( args, fmt );
			vsyslog( LOG_INFO, fmt, args );
			va_end( args );
		}
		else {
			va_list args;
			va_start( args, fmt );
			pthread_mutex_lock( &queue_mutex );
			log_entry_t* entry = NULL;
			log_queue_push_start( &log_queue, &entry );
			vsprintf( entry->msg, fmt, args );
			entry->level = LOG_INFO;
			log_queue_push_end( &log_queue );
			pthread_mutex_unlock( &queue_mutex );
			va_end( args );
		}
	}
}

void log_info(
		const char* fmt,
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
		if( !threaded_log ) {
			va_list args;
			va_start( args, fmt );
			vsyslog( LOG_INFO, fmt, args );
			va_end( args );
		}
		else {
			va_list args;
			va_start( args, fmt );
			pthread_mutex_lock( &queue_mutex );
			log_entry_t* entry = NULL;
			log_queue_push_start( &log_queue, &entry );
			vsprintf( entry->msg, fmt, args );
			entry->level = LOG_INFO;
			log_queue_push_end( &log_queue );
			pthread_mutex_unlock( &queue_mutex );
			va_end( args );
		}
	}
}

void log_warning(
		const char* fmt,
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
		if( !threaded_log ) {
			va_list args;
			va_start( args, fmt );
			vsyslog( LOG_WARNING, fmt, args );
			va_end( args );
		}
		else {
			va_list args;
			va_start( args, fmt );
			pthread_mutex_lock( &queue_mutex );
			log_entry_t* entry = NULL;
			log_queue_push_start( &log_queue, &entry );
			vsprintf( entry->msg, fmt, args );
			entry->level = LOG_WARNING;
			log_queue_push_end( &log_queue );
			pthread_mutex_unlock( &queue_mutex );
			va_end( args );
		}
	}
}

void log_error(
		const char* fmt,
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
		if( !threaded_log ) {
			va_list args;
			va_start( args, fmt );
			vsyslog( LOG_ERR, fmt, args );
			va_end( args );
		}
		else {
			va_list args;
			va_start( args, fmt );
			pthread_mutex_lock( &queue_mutex );
			log_entry_t* entry = NULL;
			log_queue_push_start( &log_queue, &entry );
			char buffer[STR_BUFFER_SIZE];
			vsprintf( buffer, fmt, args );
			entry->level = LOG_ERR;
			log_queue_push_end( &log_queue );
			pthread_mutex_unlock( &queue_mutex );
			va_end( args );
		}
	}
}

void log_run(void)
{
	threaded_log = true;
	while(true) {
		log_queue_read_start( &log_queue );
		if( log_queue_get_should_stop(&log_queue) ) {
			// before stopping the logging loop,
			// log all pending messages:
			for( int i=log_queue_get_count(&log_queue)-1; i>=0; i-- ) {
				log_entry_t* entry = log_queue_read_get_index( &log_queue, i );
				syslog(entry->level, "%s", entry->msg);
			}
			return;
		}
		log_entry_t* entry = log_queue_read_get( &log_queue );
		syslog(entry->level, "%s", entry->msg);
		log_queue_read_stop_dump( &log_queue );
	}
}

void log_stop(void)
{
	log_queue_set_should_stop( &log_queue );
	threaded_log = false;
}

DEF_SPSC_QUEUE(log_queue,log_entry_t,DB_LOG,ERR_LOG)

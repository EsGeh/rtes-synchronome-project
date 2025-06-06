#include "convert.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"
#include "lib/thread.h"


#define SERVICE_NAME "convert"

#define LOG_ERROR(fmt,...) log_error( "%-20s: " fmt, SERVICE_NAME, ## __VA_ARGS__ )
#define LOG_VERBOSE(fmt,...) log_verbose( "%-20s: " fmt, SERVICE_NAME, ## __VA_ARGS__ )
#define LOG_ERROR_STD_LIB(FUNC) LOG_ERROR("'" #FUNC "': %d - %s\n", errno, strerror(errno) )

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		LOG_ERROR("error in '%s'\n", #FUNC_CALL ); \
		return RET_FAILURE; \
	} \
}

// #define LOG_TIME(FMT,...)
#define LOG_TIME(FMT,...) log_time( "%-20s %4lu.%06lu: " FMT, SERVICE_NAME, current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )


ret_t convert_run(
		const USEC deadline_us,
		const img_format_t src_format,
		select_queue_t* input_queue,
		rgb_queue_t* rgb_queue
)
{
	thread_info( "convert" );
	select_entry_t entry;
	timeval_t current_time;
	while( true ) {
		select_queue_read_start( input_queue );
		if(
				select_queue_get_should_stop( input_queue )
				&& select_queue_get_count( input_queue ) == 0
		) {
			LOG_VERBOSE( "stopping\n" );
			break;
		}
		current_time = time_measure_current_time();
		timeval_t start_time = current_time;
		LOG_TIME( "START\n" );
		entry = *select_queue_read_get(input_queue);
		{
			/*
			log_info( "convert: frame %lu.%lu\n",
					entry.time.tv_sec,
					entry.time.tv_nsec / 1000 / 1000
			);
			*/
			rgb_entry_t* dst_entry;
			rgb_queue_push_start( rgb_queue, &dst_entry );
			dst_entry->time = entry.time;
			// TODO: fix error handling:
			if( RET_SUCCESS != image_convert_to_rgb(
					src_format,
					entry.frame.data,
					dst_entry->frame.data,
					dst_entry->frame.size
			) ) {
				LOG_ERROR("error in '%s'\n", "image_convert_to_rgb" );
				return RET_FAILURE;
			}
			rgb_queue_push_end( rgb_queue );
		}
		select_queue_read_stop_dump(input_queue);
		// log timing info:
		current_time = time_measure_current_time();
		timeval_t end_time = current_time;
		LOG_TIME( "END\n" );
		{
			timeval_t runtime;
			time_delta( &end_time, &start_time, &runtime );
			LOG_TIME( "RUNTIME: %04lu.%06lu\n",
					runtime.tv_sec,
					runtime.tv_nsec / 1000
			);
			USEC rt_us = time_us_from_timespec( &runtime );
			if( rt_us > deadline_us  ) {
				LOG_ERROR( "deadline failed %06luus > %06luus\n", rt_us , deadline_us );
				return RET_FAILURE;
			}
		}
	}
	return RET_SUCCESS;
}

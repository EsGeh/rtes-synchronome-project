#include "write_to_storage.h"

#include "exe/synchronome/queues/rgb_queue.h"
#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/semaphore.h"
#include "lib/time.h"
#include "lib/thread.h"

#include <string.h>


#define SERVICE_NAME "write_to_storage"

#define LOG_ERROR(fmt,...) log_error( "%-20s: " fmt, SERVICE_NAME , ## __VA_ARGS__ )
#define LOG_VERBOSE(fmt,...) log_verbose( "%-20s: " fmt, SERVICE_NAME, ## __VA_ARGS__ )
#define LOG_ERROR_STD_LIB(FUNC) LOG_ERROR("'" #FUNC "': %d - %s\n", errno, strerror(errno) )

#define LOG_TIME(FMT,...) log_time( "%-20s %4lu.%06lu: " FMT, SERVICE_NAME, current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return RET_FAILURE; \
	} \
}

ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		rgb_consumers_queue_t* rgb_consumers_queue,
		sem_t* other_services_done,
		const frame_size_t frame_size,
		const char* output_dir
)
{
	thread_info( SERVICE_NAME );
	char output_path[STR_BUFFER_SIZE];
	char timestamp_str[STR_BUFFER_SIZE];
	int counter = 0;
	timeval_t current_time;
	while( true ) {
		rgb_queue_read_start( rgb_queue );
		if( rgb_queue_get_should_stop( rgb_queue ) ) {
			LOG_VERBOSE( "stopping\n" );
			break;
		}
		current_time = time_measure_current_time();
		timeval_t start_time = current_time;
		LOG_TIME( "START\n" );
		snprintf(output_path, STR_BUFFER_SIZE, "%s/image%04u.ppm",
				output_dir,
				counter
		);
		{
			rgb_entry_t* entry = rgb_queue_read_get(rgb_queue);
			if( other_services_done != NULL ) {
				rgb_entry_t** consumer_entry = NULL;
				rgb_consumers_queue_push_start( rgb_consumers_queue, &consumer_entry );
				(*consumer_entry) = entry;
				rgb_consumers_queue_push_end( rgb_consumers_queue );
			}
			snprintf(timestamp_str, STR_BUFFER_SIZE, "%lu.%lu",
					entry->time.tv_sec,
					entry->time.tv_nsec / 1000 / 1000
			);
			log_info( "[Frame Count: %4u] [Image Capture Start Time: %4lu.%03lu second]\n",
					counter,
					entry->time.tv_sec,
					entry->time.tv_nsec / 1000 / 1000
			);
			// TODO: fix error handling:
			API_RUN( image_save_ppm(
				output_path,
				timestamp_str,
				entry->frame.data,
				entry->frame.size,
				frame_size.width,
				frame_size.height
			) );
		}
		if( other_services_done != NULL ) {
			API_RUN( sem_wait_nointr( other_services_done ) );
		}
		rgb_queue_read_stop_dump(rgb_queue);
		counter++;
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
		}
	}
	return RET_SUCCESS;
}

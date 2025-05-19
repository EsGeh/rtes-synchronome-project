#include "write_to_storage.h"

#include "exe/synchronome/queues/rgb_queue.h"
#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"
#include "lib/thread.h"


#define LOG_TIME(FMT,...) log_time( "write_to_storage %4lu.%06lu: " FMT, current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )

ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		const frame_size_t frame_size,
		const char* output_dir
)
{
	thread_info( "write_to_storage" );
	char output_path[STR_BUFFER_SIZE];
	char timestamp_str[STR_BUFFER_SIZE];
	int counter = 0;
	// frame_size_t frame_size = rgb_queue_get_frame_size( rgb_queue );
	timeval_t current_time;
	while( true ) {
		rgb_queue_read_start( rgb_queue );
		if( rgb_queue_get_should_stop( rgb_queue ) ) {
			log_info( "write_to_storage: stopping\n" );
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
			// rgb_queue_read_get(rgb_queue, &entry);
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
			if( RET_SUCCESS != image_save_ppm(
				output_path,
				timestamp_str,
				entry->frame.data,
				entry->frame.size,
				frame_size.width,
				frame_size.height
			) ) {
				log_error("error in '%s'\n", "image_save_ppm" ); \
				return RET_FAILURE; \
			}
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

#include "write_to_storage.h"

#include "exe/synchronome/rgb_queue.h"
#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"


ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		const char* output_dir
)
{
	char output_path[STR_BUFFER_SIZE];
	char timestamp_str[STR_BUFFER_SIZE];
	int counter = 0;
	frame_size_t frame_size = rgb_queue_get_frame_size( rgb_queue );
	while( true ) {
		rgb_queue_read_start( rgb_queue );
		if( rgb_queue_get_should_stop( rgb_queue ) ) {
			log_info( "write_to_storage: stopping\n" );
			break;
		}
		snprintf(output_path, STR_BUFFER_SIZE, "%s/image%04u.ppm",
				output_dir,
				counter
		);
		{
			rgb_entry_t* entry;
			rgb_queue_read_get(rgb_queue, &entry);
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
	}
	return RET_SUCCESS;
}

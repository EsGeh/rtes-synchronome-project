#include "convert.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"


#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return RET_FAILURE; \
	} \
}

ret_t convert_run(
		const img_format_t src_format,
		select_queue_t* input_queue,
		rgb_queue_t* rgb_queue
)
{
	select_entry_t entry;
	while( true ) {
		select_queue_read_start( input_queue );
		if( select_queue_get_should_stop( input_queue ) ) {
			log_info( "convert: stopping\n" );
			break;
		}
		select_queue_read_get(input_queue, &entry);
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
				log_error("error in '%s'\n", "image_convert_to_rgb" ); \
				return RET_FAILURE; \
			}
			rgb_queue_push_end( rgb_queue );
		}
		select_queue_read_stop_dump(input_queue);
		// frames are dumped be the `select` service!,
		// so nothing to do here!
	}
	return RET_SUCCESS;
}

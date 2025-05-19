#include "rgb_queue.h"
#include "lib/image.h"
#include "lib/output.h"

#include <string.h>


#define DBG_LOG(fmt,...) log_verbose(fmt, ## __VA_ARGS__)
#define ERR_LOG(fmt,...) { \
	log_error(fmt, ## __VA_ARGS__); \
	exit(1); \
}

DEF_SPSC_QUEUE(rgb_queue,rgb_entry_t,DBG_LOG,ERR_LOG);

void rgb_queue_init_frames( rgb_queue_t* queue, const frame_size_t size)
{
	const uint max_count = rgb_queue_get_max_count(queue);
	for( uint i=0; i<max_count; ++i ) {
		queue->entries[i].frame.data = NULL;
		queue->entries[i].frame.size = image_rgb_size( size.width, size.height );
		CALLOC(
				queue->entries[i].frame.data,
				queue->entries[i].frame.size,
				1
		);
	}
}

void rgb_queue_exit_frames( rgb_queue_t* queue )
{
	for( uint i=0; i<queue->max_count; ++i ) {
		FREE( queue->entries[i].frame.data );
	}
}

DEF_SPSC_QUEUE(rgb_consumers_queue,rgb_entry_t*,DBG_LOG,ERR_LOG);

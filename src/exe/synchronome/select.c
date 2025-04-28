#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"

#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"


ret_t select_run(
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
)
{
	acq_entry_t entry;
	timeval_t last_picked_frame = {0, 0};
	while( true ) {
		acq_queue_read_start( input_queue );
		if( acq_queue_get_should_stop( input_queue ) ) {
			log_info( "select: stopping\n" );
			break;
		}
		acq_queue_read_get(input_queue, &entry);
		timeval_t delta;
		time_delta(
				&entry.time,
				&last_picked_frame,
				&delta
		);
		USEC delta_us = time_us_from_timespec( &delta );
		//
		if( delta_us + 200 > 1000*1000 ) {
			log_info( "select: frame %lu.%lu ->\n",
					entry.time.tv_sec,
					entry.time.tv_nsec / 1000 / 1000
			);
			select_queue_push(
					output_queue,
					entry
			);
			acq_queue_read_stop_dump(input_queue);
			last_picked_frame = entry.time;
		}
		else {
			acq_queue_read_stop_dump(input_queue);
			dump_frame( entry.frame );
		}
	}
	return RET_SUCCESS;
}

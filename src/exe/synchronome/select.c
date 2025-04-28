#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"
#include "frame_acq.h"

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
	timeval_t last_picked_frame;
	last_picked_frame = time_measure_current_time();
	// char output_path[STR_BUFFER_SIZE];
	// int counter = 0;
	while( true ) {
		acq_queue_wait( input_queue );
		if( acq_queue_should_stop( input_queue ) ) {
			log_info( "select: stopping\n" );
			break;
		}
		log_info( "select: reading frame\n" );
		acq_queue_peek(input_queue, &entry);
		timeval_t delta;
		time_delta(
				&entry.time,
				&last_picked_frame,
				&delta
		);
		USEC delta_us = time_us_from_timespec( &delta );
		if( delta_us > 1000*1000 ) {
			acq_queue_pop(input_queue);
			dump_frame( entry.frame );
			select_queue_push(
					output_queue,
					entry
			);
			last_picked_frame = entry.time;
		}
		else {
			acq_queue_pop(input_queue);
			dump_frame( entry.frame );
		}
	}
	return RET_SUCCESS;
}

#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"


#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return EXIT_FAILURE; \
	} \
}

const uint max_frame_acc_count = 10;

typedef struct {
	float avg_diff;
	uint frame_count;
	timeval_t last_tick;
	timeval_t last_selected;
} select_state_t;

static select_state_t state;

// TODO:
// - if necessary, replace moving avg_diff hack
//   by a real solution using a ring buffer of img diffs

ret_t select_run(
		const img_format_t src_format,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
)
{
	while( true ) {
		// cleanup:
		if( state.frame_count >= max_frame_acc_count ) {
			state.avg_diff -= (state.avg_diff / max_frame_acc_count);
			// dump obsolete unselected frames:
			// (selected frames must be dumped by "convert")
			if(
					acq_queue_read_get(input_queue,0)->time.tv_sec != state.last_selected.tv_sec
					&& acq_queue_read_get(input_queue,0)->time.tv_nsec != state.last_selected.tv_nsec
			) {
				dump_frame( acq_queue_read_get(input_queue,0)->frame );
			}
			acq_queue_read_stop_dump(input_queue);
			state.frame_count--;
		}
		// get next frame:
		acq_queue_read_start( input_queue );
		if( acq_queue_get_should_stop( input_queue ) ) {
			log_info( "select: stopping\n" );
			break;
		}
		state.frame_count++;
		if( state.frame_count < 2 ) {
			continue;
		}
		// calc image difference:
		float diff_value;
		API_RUN(image_diff(
				src_format,
				acq_queue_read_get(input_queue,state.frame_count-1)->frame.data,
				acq_queue_read_get(input_queue,state.frame_count-2)->frame.data,
				&diff_value
		));
		log_info( "select: %lu.%lu, diff: %.3f, avg: %.3f\n",
				acq_queue_read_get(input_queue,state.frame_count-1)->time.tv_sec,
				acq_queue_read_get(input_queue,state.frame_count-1)->time.tv_nsec / 1000 / 1000,
				diff_value,
				state.avg_diff
		);
		// tick recognition:
		if( diff_value > 1.5 * state.avg_diff  ) {
			timeval_t tick_time = acq_queue_read_get(input_queue, state.frame_count-1)->time;
			// select frame between current tick and previous tick
			uint selected_frame_index;
			{
				uint last_tick_index = 0;
				for( last_tick_index=0; last_tick_index<state.frame_count; last_tick_index++ ) {
					if(
							acq_queue_read_get(input_queue, last_tick_index)->time.tv_sec == state.last_tick.tv_sec
							&& acq_queue_read_get(input_queue, last_tick_index)->time.tv_nsec == state.last_tick.tv_nsec
					) {
						break;
					}
				}
				selected_frame_index = (last_tick_index + state.frame_count-1) / 2;
			}
			state.last_selected = acq_queue_read_get(input_queue,selected_frame_index)->time;
			log_info( "tick! selected frame: %u - %lu.%lu\n",
					selected_frame_index,
					state.last_selected.tv_sec,
					state.last_selected.tv_nsec / 1000 / 1000
			);
			select_queue_push(
					output_queue,
					*acq_queue_read_get(input_queue,selected_frame_index)
			);
			state.last_tick = tick_time;
		}
		state.avg_diff += (diff_value / max_frame_acc_count);
	}
	return RET_SUCCESS;
}

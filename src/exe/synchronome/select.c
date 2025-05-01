#include "exe/synchronome/select.h"
#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/ring_buffer.h"
#include "lib/global.h"
#include "lib/time.h"


#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return EXIT_FAILURE; \
	} \
}

#define ASSERT( COND ) { \
	if( !(COND) ) { \
		log_error( "invalid parameters. condition failed: %s\n", #COND ); \
		return RET_FAILURE; \
	} \
}

// calculate avg diff over an
// interval of this size:
const uint avg_diff_count = 8;

// how much must the image diff
// be above average to be classified
// as a tick:
const float tick_threshold = 1.5f;

DECL_RING_BUFFER(diff_buffer,float)

typedef struct {
	float avg_diff;
	uint frame_count;
	int last_tick_index;
	timeval_t last_tick_time;
	// timeval_t last_selected;
	diff_buffer_t diff_buffer;
} select_state_t;

static select_state_t state;

/********************
 * Function Decls
********************/

void select_frame(
		const float clock_tick_interval,
		acq_queue_t* input_queue,
		select_queue_t* output_queue
);

/********************
 * Function Defs
********************/

/*
 * Remark:
 * this function will cleanum old frames.
 * Consumers of the output_queue must
 * be fast enough processing selected frames,
 * before they will be cleaned up to prevent
 * a segfault!
 */
ret_t select_run(
		const img_format_t src_format,
		const float acq_interval,
		const float clock_tick_interval,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
)
{
	// this val must be big enough
	// that there is always at least 2
	// tcks in an interval of this size
	// of incoming frames
	uint max_frame_acc_count;
	API_RUN( select_get_frame_acc_count(
			acq_interval,
			clock_tick_interval,
			&max_frame_acc_count
	) );
	log_info( "acq_interval: %f\n", acq_interval );
	log_info( "clock_interval: %f\n", clock_tick_interval );
	log_info( "max_frame_acc_count: %u\n", max_frame_acc_count );
	ASSERT( 1.0f/acq_interval >= 2.0 * (1.0f/clock_tick_interval) );
	ASSERT( max_frame_acc_count > 2 );
	diff_buffer_init( &state.diff_buffer, avg_diff_count );
	while( true ) {
		// cleanup:
		if( state.frame_count >= max_frame_acc_count ) {
			// dump obsolete frames:
			dump_frame( acq_queue_read_get(input_queue,0)->frame );
			acq_queue_read_stop_dump(input_queue);
			state.frame_count--;
			state.last_tick_index--;
		}
		// get next frame:
		acq_queue_read_start( input_queue );
		if( acq_queue_get_should_stop( input_queue ) ) {
			log_info( "select: stopping\n" );
			diff_buffer_exit( &state.diff_buffer );
			break;
		}
		state.frame_count++;
		if( state.frame_count < 2 ) {
			continue;
		}
		// calc image difference:
		float diff_value_f;
		API_RUN(image_diff(
				src_format,
				acq_queue_read_get(input_queue,state.frame_count-1)->frame.data,
				acq_queue_read_get(input_queue,state.frame_count-2)->frame.data,
				&diff_value_f
		));
		float diff_value = diff_value_f;
		log_info( "select: frame %lu.%lu, diff: %.3f, avg: %.3f, acc_count: %u\n",
				acq_queue_read_get(input_queue,state.frame_count-1)->time.tv_sec,
				acq_queue_read_get(input_queue,state.frame_count-1)->time.tv_nsec / 1000 / 1000,
				diff_value,
				state.avg_diff,
				state.frame_count
		);
		if(
				state.frame_count >= max_frame_acc_count
				&& diff_value > tick_threshold * state.avg_diff
		) {
			log_info( "tick!\n" );
			timeval_t tick_time = acq_queue_read_get(input_queue, state.frame_count-1)->time;
			select_frame(
					clock_tick_interval,
					input_queue,
					output_queue
			);
			state.last_tick_index = state.frame_count-1;
			state.last_tick_time = tick_time;
		}
		// update avg_diff:
		if( diff_buffer_get_count(&state.diff_buffer) < avg_diff_count ) {
			// cumulative average:
			const uint n = diff_buffer_get_count(&state.diff_buffer);
			log_info( "avg (cum): n: %u\n", n );
			state.avg_diff =
				(state.avg_diff * ((float )n) + diff_value)
				/ ((float )(n+1));
			diff_buffer_push( &state.diff_buffer, diff_value );
		}
		else {
			float oldest_diff = *diff_buffer_get( &state.diff_buffer );
			log_info( "avg (nor): oldest: %f\n", oldest_diff );
			diff_buffer_pop( &state.diff_buffer );
			diff_buffer_push( &state.diff_buffer, diff_value );
			state.avg_diff -= oldest_diff / avg_diff_count;
			state.avg_diff += diff_value / avg_diff_count;
		}
	}
	return RET_SUCCESS;
}

void select_frame(
		const float clock_tick_interval,
		acq_queue_t* input_queue,
		select_queue_t* output_queue
)
{
	timeval_t tick_time = acq_queue_read_get(input_queue, state.frame_count-1)->time;
	// if there was no previous tick:
	if( state.last_tick_time.tv_sec == 0 && state.last_tick_time.tv_nsec == 0 )
		return;
	log_info( "\tlast_tick_time: %lu.%lu\n",
			state.last_tick_time.tv_sec,
			state.last_tick_time.tv_nsec
			);
	timeval_t delta_tick;
	time_delta(
			&tick_time,
			&state.last_tick_time,
			&delta_tick
	);
	if( 
			time_us_from_timespec( &delta_tick ) > 1.5f * clock_tick_interval * 1000 * 1000
			|| state.last_tick_index < 0
	) {
		log_error( "tick missed! interval was: %lu.%lu!\n",
				delta_tick.tv_sec,
				delta_tick.tv_nsec / 1000 / 1000
		);
		return;
	}
	if( 
			(state.frame_count-1) - state.last_tick_index < 2
	) {
		log_error( "no space between ticks!\n" );
		return;
	}
	int selected_frame_index = (state.last_tick_index + state.frame_count-1) / 2;
	assert( selected_frame_index >= 0 );
	assert( selected_frame_index < (int )(state.frame_count-1) );
	log_info( "\tselected frame: %lu.%lu)\n",
			acq_queue_read_get(input_queue,selected_frame_index)->time.tv_sec,
			acq_queue_read_get(input_queue,selected_frame_index)->time.tv_nsec
	);
	select_queue_push(
			output_queue,
			*acq_queue_read_get(input_queue,selected_frame_index)
	);
}

// how many frames will be accumulated
// at maximum while running `select_run`?
ret_t select_get_frame_acc_count(
		const float acq_interval,
		const float clock_tick_interval,
		uint* frame_acc_count
)
{
	ASSERT( acq_interval > 0.001 );
	ASSERT( clock_tick_interval > 0.001 );
	ASSERT( 1.0f/acq_interval >= 2.0 * (1.0f/clock_tick_interval) );
	(*frame_acc_count) =  1.5 * clock_tick_interval / acq_interval;
	ASSERT( (*frame_acc_count) > 2 );
	return RET_SUCCESS;
}

DEF_RING_BUFFER(diff_buffer,float)

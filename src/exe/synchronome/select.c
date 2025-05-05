#include "exe/synchronome/select.h"
#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"

#include "lib/image.h"
#include "lib/output.h"
#include "lib/ring_buffer.h"
#include "lib/global.h"
#include "lib/time.h"

#include <math.h>


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

const uint adjustment_inertia = 4;
const uint sync_threshold = 4;


// calculate avg diff over an
// interval of this size:
const uint diff_buffer_max_count = 32;

DECL_RING_BUFFER(diff_buffer,float)

typedef struct {
	// how many frames
	// have been accumulated:
	uint frame_acc_count;
	// sync phase data:
	uint sync_status;
	timeval_t last_tick_time;
	// tick/phase statistics:
	timeval_t first_tick_time;
	int executed_tick_count;
	int measured_tick_count;
	float requested_drift_correction;
	// image diff statistics:
	diff_buffer_t diff_buffer;
	float avg_diff;
	float max_diff;

} select_state_t;

static select_state_t state;

/********************
 * Function Decls
********************/

void select_frame(
		const float acq_interval,
		const float clock_tick_interval,
		acq_queue_t* input_queue,
		select_queue_t* output_queue
);

/********************
 * Function Defs
********************/

#define DB_PRINT(FMT,...) log_verbose( "select %4lu.%3lu: " FMT, current_time.tv_sec, current_time.tv_nsec/1000/1000, ## __VA_ARGS__ )

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
		const float tick_threshold,
		uint select_delay,
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
	state.executed_tick_count = -1;
	state.measured_tick_count = -1;
	const float sampling_precision = acq_interval / clock_tick_interval ;
	log_verbose( "max_frame_acc_count: %4u\n", max_frame_acc_count );
	log_verbose( "sampling_precision: %f\n", sampling_precision );
	diff_buffer_init( &state.diff_buffer, diff_buffer_max_count );
	while( true ) {
		// cleanup obsolete old frames:
		if( state.frame_acc_count >= max_frame_acc_count ) {
			// dump obsolete frames:
			dump_frame( acq_queue_read_get(input_queue,0)->frame );
			acq_queue_read_stop_dump(input_queue);
			state.frame_acc_count--;
			// state.last_tick_index--;
		}
		// get next frame:
		acq_queue_read_start( input_queue );
		if( acq_queue_get_should_stop( input_queue ) ) {
			log_verbose( "select: stopping\n" );
			diff_buffer_exit( &state.diff_buffer );
			break;
		}
		state.frame_acc_count++;
		if( state.frame_acc_count < 2 ) {
			continue;
		}
		assert( state.frame_acc_count >= 2 );
		// calc image difference:
		float diff_value;
		API_RUN(image_diff(
				src_format,
				acq_queue_read_get(input_queue,state.frame_acc_count-1)->frame.data,
				acq_queue_read_get(input_queue,state.frame_acc_count-2)->frame.data,
				&diff_value
		));
		// update image diff statistics
		if( diff_buffer_get_count(&state.diff_buffer) < diff_buffer_max_count ) {
			// cumulative average:
			const uint n = diff_buffer_get_count(&state.diff_buffer);
			state.max_diff = MAX(state.max_diff,diff_value);
			state.avg_diff =
				(state.avg_diff * ((float )n) + diff_value)
				/ ((float )(n+1));
			diff_buffer_push( &state.diff_buffer, diff_value );
		}
		else {
			float oldest_diff = *diff_buffer_get( &state.diff_buffer );
			diff_buffer_pop( &state.diff_buffer );
			diff_buffer_push( &state.diff_buffer, diff_value );
			// brute force, there might be a better strategy...:
			state.max_diff = 0;
			for( uint i=0; i<diff_buffer_max_count; i++ ) {
				state.max_diff = MAX(state.max_diff, *diff_buffer_get_index(&state.diff_buffer, i) );
			}
			state.avg_diff -= oldest_diff / diff_buffer_max_count;
			state.avg_diff += diff_value / diff_buffer_max_count;
		}
		timeval_t current_time = acq_queue_read_get(input_queue,state.frame_acc_count-1)->time;
		// wait until diff statistics are stable
		if( diff_buffer_get_count(&state.diff_buffer) < diff_buffer_max_count ) {
			DB_PRINT( "collecting diff statistics: %u/%u\n",
					diff_buffer_get_count(&state.diff_buffer),
					diff_buffer_max_count
			);
			continue;
		}
		assert( diff_buffer_get_count(&state.diff_buffer) >= diff_buffer_max_count );

		const float epsilon = sampling_precision/2;
		bool tick_detected = (diff_value - state.avg_diff) > tick_threshold * (state.max_diff - state.avg_diff);

		// sync phase:
		if( state.sync_status < sync_threshold ) {
			if( tick_detected ) {
				if( state.sync_status == 0 ) {
					state.last_tick_time = current_time;
					state.sync_status++;
					continue;
				}
				timeval_t delta;
				time_delta(
						&current_time,
						&state.last_tick_time,
						&delta
				);
				float phase = ((float )time_us_from_timespec( &delta ) / 1000 / 1000) / clock_tick_interval;
				assert( phase > 0 );
				if( phase > 1 - epsilon && phase < 1 + epsilon) {
					state.sync_status++;
					if( state.sync_status == sync_threshold ) {
						// initialize next phase:
						state.first_tick_time = current_time;
						/* test drift correction: */
						time_add_us(
								&state.first_tick_time,
								-sampling_precision * clock_tick_interval * 1000 * 1000,
								&state.first_tick_time
						);
						state.executed_tick_count = 0;
						state.measured_tick_count = 0;
						DB_PRINT( "TICK 0\n" );
					}
					state.last_tick_time = current_time;
					continue;
				}
				state.sync_status = 0;
				state.last_tick_time = current_time;
			}
			DB_PRINT( "sync: %u/%u\n",
					state.sync_status,
					sync_threshold
			);
			continue;
		}

		// phase [0..1] - how much time has passed since last (executed) tick?
		float phase = 0;
		{
			timeval_t last_executed_tick_time;
			timeval_t clock_tick_interval_;
			time_timespec_from_us(
					clock_tick_interval * 1000 * 1000,
					&clock_tick_interval_
			);
			time_mul_i(
					&clock_tick_interval_,
					state.executed_tick_count,
					&last_executed_tick_time
			);
			time_add(
					&state.first_tick_time,
					&last_executed_tick_time,
					&last_executed_tick_time
			);
			timeval_t delta;
			time_delta(
					&current_time,
					&last_executed_tick_time,
					&delta
			);
			phase = ((float )time_us_from_timespec( &delta ) / 1000 / 1000) / clock_tick_interval;
		}

		// autonomously execute tick every clock_tick_rate:
		if( phase > 1 - epsilon ) {
			state.executed_tick_count++;
			phase -= 1;
			DB_PRINT( "TICK %4u!\n",
					state.executed_tick_count
			);
		}

		assert( phase <= 1.0 );

		// register detected ticks and possibl drift:
		if( tick_detected ) {
			// we are urgently wating to detect a tick already executed:
			if( state.measured_tick_count == state.executed_tick_count - 1 ) {
				if( phase < 0 + epsilon ) {
					// measured tick is on time for the latest executed tick:
					DB_PRINT( "~TICK %4u, phase: %f\n",
							state.measured_tick_count+1,
							phase
					);
				}
				else if( phase < 0 + sampling_precision + epsilon ) {
					// measured tick is 1 sample late for the latest executed tick:
					state.requested_drift_correction += + sampling_precision / adjustment_inertia;
					DB_PRINT( "~TICK %4u, phase: %f<- (late)\n",
							state.measured_tick_count+1,
							phase
					);
				}
				else {
					DB_PRINT( "~TICK %4u, phase: %f<-- (TOO LATE!)\n",
							state.measured_tick_count+1,
							phase
					);
				}
			}
			// we are "even" so the tick can only be for the next predicted tick:
			else if( state.measured_tick_count == state.executed_tick_count ) {
				if( 1 - phase < sampling_precision + epsilon ) {
					// measured tick is 1 sample ahead of the upcoming predicted tick:
					state.requested_drift_correction += - sampling_precision / adjustment_inertia;
					DB_PRINT( "~TICK %4u, phase: %f-> (early)\n",
							state.measured_tick_count+1,
							phase
					);
				}
				else {
					DB_PRINT( "~TICK %4u, phase: %f--> (TOO EARLY!)\n",
							state.measured_tick_count+1,
							phase
					);
				}
			}
			else {
				DB_PRINT( "~TICK %4u, phase: %f (?)\n",
						state.measured_tick_count+1,
						phase
				);
			}
			state.measured_tick_count++;
		}

		// counteract drift if necessary:
		if( phase > sampling_precision - epsilon && phase < sampling_precision + epsilon ) {
			// we have measured ticks for every executed tick
			if(
					state.measured_tick_count == state.executed_tick_count
					// only exception:
					// 	if we have sampling rate = 2*clock_tick_rate,
					// 	it can be that we have already seen a (1 frame early) measurement
					// 	for the next tick to be executed
					|| ( fabsf(sampling_precision - 0.5f) < 0.01
						&& state.measured_tick_count == state.executed_tick_count+1
					)
			) {
				DB_PRINT( "ADJUST requested: %f\n", state.requested_drift_correction );
				if( fabsf(state.requested_drift_correction) > sampling_precision - epsilon ) {
					float adjustment = copysign(
							sampling_precision * clock_tick_interval, // value
							state.requested_drift_correction // sign
					);
					DB_PRINT( "ADJUST: %f\n", adjustment );
					time_add_us(
							&state.first_tick_time,
							adjustment * 1000 * 1000,
							&state.first_tick_time
					);
					state.requested_drift_correction = 0;
				}
			}
			else {
				// ERROR:
				if( state.measured_tick_count > state.executed_tick_count ) {
					log_error( "select %4lu.%3lu: superfluous TICKs: %d > %d!\n",
						current_time.tv_sec,
						current_time.tv_nsec /1000/1000,
						state.measured_tick_count,
						state.executed_tick_count
					);
				}
				else {
					// missing detected ticks!
					log_error( "select %4lu.%3lu: missing TICKs: %d < %d!\n",
						current_time.tv_sec,
						current_time.tv_nsec/1000/1000,
						state.measured_tick_count,
						state.executed_tick_count
					);
				}
				// simple error recovery measure:
				state.measured_tick_count = state.executed_tick_count;
				state.requested_drift_correction = 0;
			}
		}

		// if this was a "tick" frame:
		if( phase < 0 + epsilon ) {
			if( time_us_from_timespec( &current_time ) < (USEC )select_delay * 1000*1000 ) {
				continue;
			}
			// select frame from recent recorded
			// frames:
			select_frame(
					acq_interval,
					clock_tick_interval,
					input_queue,
					output_queue
			);
		}

	}
	return RET_SUCCESS;
}

void select_frame(
		const float acq_interval,
		const float clock_tick_interval,
		acq_queue_t* input_queue,
		select_queue_t* output_queue
)
{
	const float sampling_frequency = clock_tick_interval / acq_interval;
	int selected_frame_index = state.frame_acc_count-1 - (int )ceilf(0.5f * sampling_frequency);
	assert( selected_frame_index >= 0 );
	assert( selected_frame_index < (int )(state.frame_acc_count-1) );
	log_verbose( "\tselected frame: %lu.%lu)\n",
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

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
	diff_buffer_t diff_buffer;
	float avg_diff;
	float max_diff;
} diff_statistics_t;

typedef struct {
	uint sync_status;
	timeval_t last_tick_time;
} synchronize_state_t;

typedef struct {
	timeval_t first_tick_time;
	int executed_tick_count;
	int measured_tick_count;
	float requested_drift_correction;
} tick_parser_state_t;

typedef struct {
	// how many frames
	// have been accumulated:
	uint frame_acc_count;
	// sync phase data:
	synchronize_state_t synchronize_state;
	// tick/phase statistics:
	tick_parser_state_t tick_parser_state;
	// selection data:
	int last_tick_index;
	// image diff statistics:
	diff_statistics_t diff_statistics;

} select_state_t;

// static select_state_t state;

/********************
 * Function Decls
********************/

void cleanup_frames(
		const uint max_frame_acc_count,
		dump_frame_func_t dump_frame,
		uint* frame_acc_count,
		acq_queue_t* input_queue
);

int update_img_diff(
		const timeval_t frame_time,
		float diff_value,
		diff_statistics_t* diff_statistics
);

int synchronize(
		const timeval_t frame_time,
		const bool tick_detected,
		const float acq_interval,
		const float clock_tick_interval,
		synchronize_state_t* state
);

float calc_phase(
		const tick_parser_state_t* state,
		const timeval_t* frame_time,
		const float clock_tick_interval
);

void select_frame(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const acq_queue_t* input_queue,
		select_state_t* state,
		select_queue_t* output_queue
);


void tick_parser_init(
		const timeval_t frame_time,
		tick_parser_state_t* state
);

void tick_parser(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const float phase,
		const bool tick_detected,
		tick_parser_state_t* state
);

/********************
 * Function Defs
********************/

static timeval_t current_time;

#define VERBOSE_PRINT_FRAME(FMT,...) log_verbose( "select %4lu.%06lu, frame: %4lu.%06lu: " FMT, \
		current_time.tv_sec, current_time.tv_nsec/1000, \
		frame_time.tv_sec, frame_time.tv_nsec/1000, ## __VA_ARGS__ )

#define VERBOSE_PRINT(FMT,...) log_verbose( "select %4lu.%06lu: " FMT, \
		current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )

#define LOG_TIME(FMT,...) log_time( "select %4lu.%06lu: " FMT, current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )

#define LOG_TIME_END() \
		current_time = time_measure_current_time(); \
		timeval_t end_time = current_time; \
		LOG_TIME( "END\n" ); \
		{ \
			timeval_t runtime; \
			time_delta( &end_time, &start_time, &runtime ); \
			LOG_TIME( "RUNTIME: %04lu.%06lu\n", \
					runtime.tv_sec, \
					runtime.tv_nsec / 1000 \
			); \
		}

/* Remark:
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
	current_time = time_measure_current_time();
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
	static select_state_t state;
	state.tick_parser_state.executed_tick_count = -1;
	state.tick_parser_state.measured_tick_count = -1;
	const float sampling_precision = acq_interval / clock_tick_interval ;
	const float epsilon = sampling_precision/2;
	VERBOSE_PRINT( "max_frame_acc_count: %4u\n", max_frame_acc_count );
	VERBOSE_PRINT( "sampling_precision: %f\n", sampling_precision );
	diff_buffer_init( &state.diff_statistics.diff_buffer, diff_buffer_max_count );
	while( true ) {
		// cleanup obsolete old frames:
		cleanup_frames( max_frame_acc_count, dump_frame, &state.frame_acc_count, input_queue );
		// get next frame:
		acq_queue_read_start( input_queue );
		if( acq_queue_get_should_stop( input_queue ) ) {
			VERBOSE_PRINT( "select: stopping\n" );
			diff_buffer_exit( &state.diff_statistics.diff_buffer );
			break;
		}
		timeval_t frame_time = acq_queue_read_get(input_queue,state.frame_acc_count-1)->time;
		current_time = time_measure_current_time();
		timeval_t start_time = current_time;
		LOG_TIME( "START\n" );
		state.frame_acc_count++;
		if( state.frame_acc_count < 2 ) {
			LOG_TIME_END()
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
		{
			int ret = update_img_diff( frame_time, diff_value, &state.diff_statistics);
			if( -1 ==  ret ) {
				LOG_TIME_END()
				continue;
			}
		}
		assert( diff_buffer_get_count(&state.diff_statistics.diff_buffer) >= diff_buffer_max_count );

		bool tick_detected = (diff_value - state.diff_statistics.avg_diff) > tick_threshold * (state.diff_statistics.max_diff - state.diff_statistics.avg_diff);

		// initial sync phase:
		{
			int ret = synchronize(
					frame_time,
					tick_detected,
					acq_interval,
					clock_tick_interval,
					&state.synchronize_state
			);
			if( -1 == ret ) {
				LOG_TIME_END()
				continue;
			}
			if( -2 == ret ) {
				// initialize next phase:
				tick_parser_init(
						frame_time,
						&state.tick_parser_state
				);
				state.last_tick_index = state.frame_acc_count-1;
				VERBOSE_PRINT_FRAME( "TICK 0\n" );
#if TEST_DRIFT_AHEAD
				state.last_tick_index--;
#elif TEST_DRIFT_BEHIND
				state.last_tick_index -= 2;
#endif
				state.last_tick_index--;
				LOG_TIME_END()
				continue;
			}
		}

		// phase [0..1] - how much time has passed since last (executed) tick?
		float phase = calc_phase( &state.tick_parser_state, &frame_time, clock_tick_interval );

		// autonomously execute tick every clock_tick_rate:
		if( phase > 1 - epsilon ) {
			state.tick_parser_state.executed_tick_count++;
			phase -= 1;
			VERBOSE_PRINT_FRAME( "TICK %4u!\n",
					state.tick_parser_state.executed_tick_count
			);
		}

		assert( phase <= 1.0 );

		// register measured ticks and if the measurements
		// pass certain sanity criteria,
		// counteract drift if necessary
		tick_parser(
				frame_time,
				acq_interval,
				clock_tick_interval,
				phase,
				tick_detected,
				&state.tick_parser_state
		);

		// if this was a "tick" frame:
		if( phase < 0 + epsilon ) {
			// do nothing if initial delay is not yet over:
			if( time_us_from_timespec( &frame_time ) < (USEC )select_delay * 1000*1000 ) {
				state.last_tick_index = state.frame_acc_count-1;
				LOG_TIME_END()
				continue;
			}
			// select frame from recent recorded frames:
			select_frame(
					frame_time,
					acq_interval,
					clock_tick_interval,
					input_queue,
					&state,
					output_queue
			);
		}
		state.last_tick_index--;
	}
	return RET_SUCCESS;
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

void select_frame(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const acq_queue_t* input_queue,
		select_state_t* state,
		select_queue_t* output_queue
)
{
	int selected_frame_index = (state->last_tick_index + state->frame_acc_count-1) / 2;
	assert( selected_frame_index >= 0 );
	assert( selected_frame_index < (int )(state->frame_acc_count-1) );
	VERBOSE_PRINT_FRAME( "\tselected frame: %4lu.%06lu)\n",
			acq_queue_read_get(input_queue,selected_frame_index)->time.tv_sec,
			acq_queue_read_get(input_queue,selected_frame_index)->time.tv_nsec
	);
	select_queue_push(
			output_queue,
			*acq_queue_read_get(input_queue,selected_frame_index)
	);
	state->last_tick_index = state->frame_acc_count-1;
}


// cleanup obsolete old frames:
void cleanup_frames(
		const uint max_frame_acc_count,
		dump_frame_func_t dump_frame,
		uint* frame_acc_count,
		acq_queue_t* input_queue
) {
	if( (*frame_acc_count) >= max_frame_acc_count ) {
		// dump obsolete frames:
		dump_frame( acq_queue_read_get(input_queue,0)->frame );
		acq_queue_read_stop_dump(input_queue);
		(*frame_acc_count)--;
	}
}

int update_img_diff(
		const timeval_t frame_time,
		const float diff_value,
		diff_statistics_t* diff_statistics
) {
	if( diff_buffer_get_count(&diff_statistics->diff_buffer) < diff_buffer_max_count ) {
		// cumulative average:
		const uint n = diff_buffer_get_count(&diff_statistics->diff_buffer);
		diff_statistics->max_diff = MAX(diff_statistics->max_diff,diff_value);
		diff_statistics->avg_diff =
			(diff_statistics->avg_diff * ((float )n) + diff_value)
			/ ((float )(n+1));
		diff_buffer_push( &diff_statistics->diff_buffer, diff_value );
	}
	else {
		float oldest_diff = *diff_buffer_get( &diff_statistics->diff_buffer );
		diff_buffer_pop( &diff_statistics->diff_buffer );
		diff_buffer_push( &diff_statistics->diff_buffer, diff_value );
		// brute force, there might be a better strategy...:
		diff_statistics->max_diff = 0;
		for( uint i=0; i<diff_buffer_max_count; i++ ) {
			diff_statistics->max_diff = MAX(diff_statistics->max_diff, *diff_buffer_get_index(&diff_statistics->diff_buffer, i) );
		}
		diff_statistics->avg_diff -= oldest_diff / diff_buffer_max_count;
		diff_statistics->avg_diff += diff_value / diff_buffer_max_count;
	}
	// wait until diff statistics are stable
	if( diff_buffer_get_count(&diff_statistics->diff_buffer) < diff_buffer_max_count ) {
		VERBOSE_PRINT_FRAME( "collect diff statistics: %u/%u\n",
				diff_buffer_get_count(&diff_statistics->diff_buffer),
				diff_buffer_max_count
				);
		return -1;
	}
	return 0;
}

int synchronize(
		const timeval_t frame_time,
		const bool tick_detected,
		const float acq_interval,
		const float clock_tick_interval,
		synchronize_state_t* state
) {
	const float sampling_precision = acq_interval / clock_tick_interval ;
	const float epsilon = sampling_precision/2;
	if( state->sync_status >= sync_threshold ) {
		return 0;
	}
	if( tick_detected ) {
		// first tick seen:
		if( state->sync_status == 0 ) {
			VERBOSE_PRINT_FRAME( "sync: %u/%u\n",
					state->sync_status,
					sync_threshold
			);
			state->last_tick_time = frame_time;
			state->sync_status++;
			return -1;
		}
		timeval_t delta;
		time_delta(
				&frame_time,
				&state->last_tick_time,
				&delta
		);
		float phase = ((float )time_us_from_timespec( &delta ) / 1000 / 1000) / clock_tick_interval;
		assert( phase > 0 );
		// if this tick is out of sync, reset and begin anew:
		if( !(phase > 1 - epsilon && phase < 1 + epsilon) ) {
			state->sync_status = 0;
			state->last_tick_time = frame_time;
			return -1;
		}
		state->sync_status++;
		if( state->sync_status == sync_threshold ) {
			VERBOSE_PRINT_FRAME( "sync: %u/%u\n",
					state->sync_status,
					sync_threshold
			);
			return -2;
		}
		state->last_tick_time = frame_time;
		VERBOSE_PRINT_FRAME( "sync: %u/%u\n",
				state->sync_status,
				sync_threshold
		);
		return -1;
	}
	return -1;
}

float calc_phase(
		const tick_parser_state_t* state,
		const timeval_t* frame_time,
		const float clock_tick_interval
) {
	timeval_t last_executed_tick_time;
	timeval_t clock_tick_interval_;
	time_timespec_from_us(
			clock_tick_interval * 1000 * 1000,
			&clock_tick_interval_
	);
	time_mul_i(
			&clock_tick_interval_,
			state->executed_tick_count,
			&last_executed_tick_time
	);
	time_add(
			&state->first_tick_time,
			&last_executed_tick_time,
			&last_executed_tick_time
	);
	timeval_t delta;
	time_delta(
			frame_time,
			&last_executed_tick_time,
			&delta
	);
	return ((float )time_us_from_timespec( &delta ) / 1000 / 1000) / clock_tick_interval;
}

void tick_parser_init(
		const timeval_t frame_time,
		tick_parser_state_t* state
)
{
	state->first_tick_time = frame_time;
	state->executed_tick_count = 0;
	state->measured_tick_count = 0;
#if TEST_DRIFT_AHEAD
	/* start with internal clock early
	 * to test resync:
	 */
	time_add_us(
			&state.first_tick_time,
			- sampling_precision * clock_tick_interval * 1000 * 1000,
			&state.first_tick_time
			);
	VERBOSE_PRINT_FRAME( "EMULATE TICK 0 <- 0.333\n" );
#elif TEST_DRIFT_BEHIND
	/* start with internal clock lagging
	 * to test resync:
	 */
	time_add_us(
			&state.first_tick_time,
			-2* sampling_precision * clock_tick_interval * 1000 * 1000,
			&state.first_tick_time
			);
	VERBOSE_PRINT_FRAME( "EMULATE TICK 0 <- 0.666\n" );
#endif
}

// register detected ticks and drift:
void tick_parser(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const float phase,
		const bool tick_detected,
		tick_parser_state_t* state
)
{
	const float sampling_precision = acq_interval / clock_tick_interval ;
	const float epsilon = sampling_precision/2;
	// 1. register measured ticks:
	if( tick_detected ) {
		// we are urgently wating to detect a tick already executed:
		if( state->measured_tick_count == state->executed_tick_count - 1 ) {
			if( phase < 0 + epsilon ) {
				// measured tick is on time for the latest executed tick:
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f\n",
						state->measured_tick_count+1,
						phase
				);
			}
			else if( phase < 0 + sampling_precision + epsilon ) {
				// measured tick is 1 sample late for the latest executed tick:
				state->requested_drift_correction += + sampling_precision / adjustment_inertia;
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f<- (late)\n",
						state->measured_tick_count+1,
						phase
				);
			}
			else {
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f<-- (TOO LATE!)\n",
						state->measured_tick_count+1,
						phase
				);
			}
		}
		// we are "even" so the tick can only be for the next predicted tick:
		else if( state->measured_tick_count == state->executed_tick_count ) {
			if( 1 - phase < sampling_precision + epsilon ) {
				// measured tick is 1 sample ahead of the upcoming predicted tick:
				state->requested_drift_correction += - sampling_precision / adjustment_inertia;
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f-> (early)\n",
						state->measured_tick_count+1,
						phase
				);
			}
			else {
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f--> (TOO EARLY!)\n",
						state->measured_tick_count+1,
						phase
				);
			}
		}
		else {
			VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %f (?)\n",
					state->measured_tick_count+1,
					phase
			);
		}
		state->measured_tick_count++;
	}
	// at exactly 1 sample after an (internal) tick,
	// we take stocK:
	// - was there exactly 1 measured tick?
	// - was the measured tick on time?
	if( phase > sampling_precision - epsilon && phase < sampling_precision + epsilon ) {
		// we have measured ticks for every executed tick
		if( state->measured_tick_count == state->executed_tick_count ) {
			VERBOSE_PRINT_FRAME( "ADJUST requested: %f\n", state->requested_drift_correction );
			if( fabsf(state->requested_drift_correction) > sampling_precision - epsilon ) {
				float adjustment = copysign(
						sampling_precision * clock_tick_interval, // value
						state->requested_drift_correction // sign
				);
				VERBOSE_PRINT_FRAME( "ADJUST: %f\n", adjustment );
				time_add_us(
						&state->first_tick_time,
						adjustment * 1000 * 1000,
						&state->first_tick_time
				);
				state->requested_drift_correction = 0;
			}
		}
		else {
			// ERROR:
			if( state->measured_tick_count > state->executed_tick_count ) {
				log_error( "select %4lu.%3lu: superfluous TICKs: %d > %d!\n",
					frame_time.tv_sec,
					frame_time.tv_nsec /1000/1000,
					state->measured_tick_count,
					state->executed_tick_count
				);
			}
			else {
				// missing detected ticks!
				log_error( "select %4lu.%3lu: missing TICKs: %d < %d!\n",
					frame_time.tv_sec,
					frame_time.tv_nsec/1000/1000,
					state->measured_tick_count,
					state->executed_tick_count
				);
			}
			// simple error recovery measure:
			state->measured_tick_count = state->executed_tick_count;
			state->requested_drift_correction = 0;
		}
	}
}

DEF_RING_BUFFER(diff_buffer,float)

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

const uint adjustment_inertia = 4;
const uint sync_threshold = 4;


// calculate avg diff over an
// interval of this size:
#define diff_buffer_max_count 32

DECL_RING_BUFFER(diff_buffer,float)

typedef struct {
	diff_buffer_t diff_buffer;
	float median_diff;
	float avg_diff;
	float max_diff;
	float min_diff;
} diff_statistics_t;

typedef struct {
	uint sync_status;
	timeval_t last_tick_time;
} synchronize_state_t;

typedef struct {
	int frame_index;
	int measured_tick_count;
	int drift_correction;
	int requested_drift_correction;
	int select_prefer_latest;
	bool sleep_one_frame;
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

void tick_parser_init(
		const timeval_t frame_time,
		tick_parser_state_t* state
);

int tick_parser(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const bool tick_detected,
		const uint frame_acc_count,
		tick_parser_state_t* state,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		int* last_tick_index
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
	state.tick_parser_state.measured_tick_count = -1;
	const int sampling_resolution = clock_tick_interval / acq_interval ;
	VERBOSE_PRINT( "max_frame_acc_count: %4u\n", max_frame_acc_count );
	VERBOSE_PRINT( "sampling_resolution: %d\n", sampling_resolution );
	diff_buffer_init( &state.diff_statistics.diff_buffer, diff_buffer_max_count );
	state.diff_statistics.median_diff = -1;
	state.diff_statistics.max_diff = 0;
	state.diff_statistics.min_diff = 100;
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
		current_time = time_measure_current_time();
		timeval_t start_time = current_time;
		LOG_TIME( "START\n" );
		state.frame_acc_count++;
		timeval_t frame_time = acq_queue_read_get(input_queue,state.frame_acc_count-1)->time;
		if( state.frame_acc_count < 2 ) {
			LOG_TIME_END()
			continue;
		}
		ASSERT( state.frame_acc_count >= 2 );
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
		ASSERT( diff_buffer_get_count(&state.diff_statistics.diff_buffer) >= diff_buffer_max_count );

		bool tick_detected = ((diff_value - state.diff_statistics.median_diff) / (state.diff_statistics.max_diff - state.diff_statistics.median_diff)) > tick_threshold;

		// initial sync phase:
		{
			int ret = synchronize(
					frame_time,
					tick_detected,
					acq_interval,
					clock_tick_interval,
					&state.synchronize_state
			);
			if( 1 == ret ) {
				diff_buffer_exit( &state.diff_statistics.diff_buffer );
				return RET_FAILURE;
			}
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
				state.last_tick_index--;
				LOG_TIME_END()
				continue;
			}
		}
		state.tick_parser_state.frame_index ++;

		// autonomously execute tick every clock_tick_rate:
		if( state.tick_parser_state.frame_index % sampling_resolution == 0 ) {
			VERBOSE_PRINT_FRAME( "TICK %4u!\n",
					state.tick_parser_state.frame_index / sampling_resolution
			);
		}

		// register measured ticks and if the measurements
		// pass certain sanity criteria,
		// counteract drift if necessary
		if( 1 == tick_parser(
				frame_time,
				acq_interval,
				clock_tick_interval,
				tick_detected,
				state.frame_acc_count,
				&state.tick_parser_state,
				input_queue,
				output_queue,
				&state.last_tick_index
		)) {
			diff_buffer_exit( &state.diff_statistics.diff_buffer );
			return RET_FAILURE;
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
	const int sampling_resolution = clock_tick_interval / acq_interval ;
	ASSERT( acq_interval > 0.001 );
	ASSERT( clock_tick_interval > 0.001 );
	ASSERT( sampling_resolution >= 3 );
	(*frame_acc_count) =
		sampling_resolution
		+ 1 // frame select is one frame AFTER internal tick
		+ 1 // one frame deadline for further processing
		+ 2 // some headroom, just to be safe...
	;
	ASSERT( (*frame_acc_count) > 2 );
	return RET_SUCCESS;
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

int compare_float( const void* p1, const void* p2 ) {
	if( *((const float* )p1) < *((const float* ) p2) ) {
		return -1;
	}
	else if( *((const float* )p1) > *((const float* ) p2) ) {
		return 1;
	}
	return 0;
}

int update_img_diff(
		const timeval_t frame_time,
		const float diff_value,
		diff_statistics_t* diff_statistics
) {
	static float sorted[diff_buffer_max_count];
	if( diff_buffer_get_count(&diff_statistics->diff_buffer) < diff_buffer_max_count ) {
		// cumulative average:
		const uint n = diff_buffer_get_count(&diff_statistics->diff_buffer);
		diff_statistics->max_diff = MAX(diff_statistics->max_diff,diff_value);
		diff_statistics->min_diff = MIN(diff_statistics->min_diff,diff_value);
		diff_statistics->avg_diff =
			(diff_statistics->avg_diff * ((float )n) + diff_value)
			/ ((float )(n+1));
		diff_buffer_push( &diff_statistics->diff_buffer, diff_value );
	}
	else {
		float oldest_diff = *diff_buffer_get( &diff_statistics->diff_buffer );
		diff_buffer_pop( &diff_statistics->diff_buffer );
		diff_buffer_push( &diff_statistics->diff_buffer, diff_value );
		// not very efficient, but works:
		for( uint i=0; i<diff_buffer_max_count; i++ ) {
			sorted[i] = *diff_buffer_get_index(&diff_statistics->diff_buffer, i);
		}
		qsort(&sorted, diff_buffer_max_count, sizeof(float), compare_float);
		diff_statistics->min_diff = sorted[0];
		diff_statistics->max_diff = sorted[diff_buffer_max_count-1-2];
		diff_statistics->median_diff = sorted[diff_buffer_max_count/2];
		diff_statistics->avg_diff -= oldest_diff / diff_buffer_max_count;
		diff_statistics->avg_diff += diff_value / diff_buffer_max_count;
	}
	VERBOSE_PRINT_FRAME( "diff value: %f, median: %f, range: %f...%f, avg: %f\n",
			(diff_value  - diff_statistics->median_diff) / (diff_statistics->max_diff - diff_statistics->median_diff),
			diff_statistics->median_diff,
			diff_statistics->min_diff,
			diff_statistics->max_diff,
			diff_statistics->avg_diff
	);
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
			state->last_tick_time = frame_time;
			state->sync_status++;
			VERBOSE_PRINT_FRAME( "sync: %u/%u\n",
					state->sync_status,
					sync_threshold
			);
			return -1;
		}
		timeval_t delta;
		time_delta(
				&frame_time,
				&state->last_tick_time,
				&delta
		);
		float phase = ((float )time_us_from_timespec( &delta ) / 1000 / 1000) / clock_tick_interval;
		ASSERT( phase > 0 );
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

void tick_parser_init(
		const timeval_t frame_time,
		tick_parser_state_t* state
)
{
	state->frame_index = 0;
	state->measured_tick_count = 0;
	state->drift_correction = 0;
	state->requested_drift_correction = 0;
	state->select_prefer_latest = 0;
	state->sleep_one_frame = false;
}

// register detected ticks and drift:
int tick_parser(
		const timeval_t frame_time,
		const float acq_interval,
		const float clock_tick_interval,
		const bool tick_detected,
		const uint frame_acc_count,
		tick_parser_state_t* state,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		int* last_tick_index
)
{
	const int sampling_resolution = clock_tick_interval / acq_interval;
	const int tick_count = state->frame_index / sampling_resolution;
	const int phase = state->frame_index % sampling_resolution;
	// 1. register measured ticks:
	if( tick_detected ) {
		// we are wating to detect a tick already executed:
		if( state->measured_tick_count == tick_count - 1 ) {
			if( phase == 0 ) {
				// measured tick is on time for the latest executed tick:
				// vote against drift correction:
				if( state->requested_drift_correction < 0 ) {
					state->requested_drift_correction += 1;
				}
				else if( state->requested_drift_correction > 0 ) {
					state->requested_drift_correction -= 1;
				}
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d\n",
						state->measured_tick_count+1,
						phase
				);
			}
			else if( phase == 1 ) {
				// measured tick is 1 sample late for the latest executed tick:
				state->requested_drift_correction += 1;
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d<- (late)\n",
						state->measured_tick_count+1,
						phase
				);
				state->select_prefer_latest = 1;
			}
			else {
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d<-- (TOO LATE!)\n",
						state->measured_tick_count+1,
						phase
				);
			}
		}
		// we are "even" so the tick can only be for the next predicted tick:
		else if( state->measured_tick_count == tick_count ) {
			if( phase == sampling_resolution - 1 ) {
				// measured tick is 1 sample ahead of the upcoming predicted tick:
				state->requested_drift_correction -= 1;
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d-> (early)\n",
						state->measured_tick_count+1,
						phase
				);
			}
			else {
				VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d--> (TOO EARLY!)\n",
						state->measured_tick_count+1,
						phase
				);
			}
		}
		else {
			VERBOSE_PRINT_FRAME( "~TICK %4u, phase: %d (?)\n",
					state->measured_tick_count+1,
					phase
			);
		}
		state->measured_tick_count++;
	}
	// 2. at exactly 1 sample after an (internal) tick,
	// we take stocK:
	// - was there exactly 1 measured tick?
	// - was the detected tick on time?
	if( phase == 1 ) {
		if( state->sleep_one_frame ) {
			state->sleep_one_frame = false;
			return 0;
		}
		int selected_frame_index = ( (*last_tick_index) + frame_acc_count-2 ) / 2;
		// we have detected ticks for every executed tick
		if( state->measured_tick_count == tick_count ) {
			VERBOSE_PRINT_FRAME( "ADJUST requested: %d/%d\n",
					state->requested_drift_correction,
					adjustment_inertia
			);
			if( (uint )abs(state->requested_drift_correction) > 0 ) {
				selected_frame_index += state->select_prefer_latest;
				if( (uint )abs(state->requested_drift_correction) >= adjustment_inertia ) {
					if( state->requested_drift_correction < 0 ) {
						state->frame_index += 1;
						VERBOSE_PRINT_FRAME( "ADJUST applied: %d\n", +1);
					}
					else {
						state->frame_index -= 1;
						VERBOSE_PRINT_FRAME( "ADJUST applied: %d\n", -1);
						state->sleep_one_frame = true;
					}
					state->requested_drift_correction = 0;
				}
			}
		}
		else {
			// ERROR:
			if( state->measured_tick_count > tick_count ) {
				log_error( "select %4lu.%3lu: superfluous TICKs: %d > %d!\n",
					frame_time.tv_sec,
					frame_time.tv_nsec /1000/1000,
					state->measured_tick_count,
					tick_count
				);
			}
			else {
				// missing detected ticks!
				log_error( "select %4lu.%3lu: missing TICKs: %d < %d!\n",
					frame_time.tv_sec,
					frame_time.tv_nsec/1000/1000,
					state->measured_tick_count,
					tick_count
				);
			}
			// simple error recovery measure:
			state->measured_tick_count = tick_count;
			state->requested_drift_correction = 0;
		}
		if( tick_count != 0 ) {
			ASSERT( selected_frame_index >= 0 );
			ASSERT( selected_frame_index < (int )(frame_acc_count-1) );
			VERBOSE_PRINT_FRAME( "\tselected frame: %4lu.%03lu\n",
				acq_queue_read_get(input_queue,selected_frame_index)->time.tv_sec,
				acq_queue_read_get(input_queue,selected_frame_index)->time.tv_nsec/1000/1000
			);
			select_queue_push(
					output_queue,
					*acq_queue_read_get(input_queue,selected_frame_index)
			);
			(*last_tick_index) = frame_acc_count-2;
		}
		// reset:
		state->select_prefer_latest = 0;
	}
	return 0;
}

DEF_RING_BUFFER(diff_buffer,float)

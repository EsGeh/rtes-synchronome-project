#include "frame_acq.h"

#include "exe/synchronome/acq_queue.h"
#include "lib/camera.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"
#include "lib/thread.h"
#include "lib/ring_buffer.h"
#include "lib/semaphore.h"

#include <pthread.h>
#include <string.h>


/********************
 * Function Decls
********************/

int set_camera_format(
		camera_t* camera,
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval
);

DECL_RING_BUFFER(frames,frame_buffer_t)

/********************
 * Global Data:
********************/

typedef struct {
	frames_t frames;
	pthread_mutex_t mutex;
} frame_dumpster_t;

// frames that may be returned to the camera:
static frame_dumpster_t frame_dumpster;

/********************
 * Function Defs
********************/

#define CAMERA_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		log_error("%s\n", camera_error() ); \
		return RET_FAILURE; \
	} \
}

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return RET_FAILURE; \
	} \
}

#define LOG_TIME(FMT,...) log_time( "frame_acq %4lu.%06lu: " FMT, current_time.tv_sec, current_time.tv_nsec/1000, ## __VA_ARGS__ )
static timeval_t current_time;

ret_t frame_acq_init(
		camera_t* camera,
		const char* dev_name,
		const uint buffer_size,
		const pixel_format_t required_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval
)
{
	CAMERA_RUN(camera_init(
			camera,
			dev_name
	));
	CAMERA_RUN(set_camera_format(
				camera,
				required_format,
				size,
				acq_interval
	));
	CAMERA_RUN( camera_init_buffer(
			camera,
			buffer_size
	));
	CAMERA_RUN( camera_stream_start( camera ));
	frames_init( &frame_dumpster.frames, buffer_size );
	pthread_mutex_init( &frame_dumpster.mutex, 0);
	return RET_SUCCESS;
}

ret_t frame_acq_exit(
		camera_t* camera
)
{
	CAMERA_RUN( camera_stream_stop( camera ));
	pthread_mutex_destroy( &frame_dumpster.mutex);
	frames_exit( &frame_dumpster.frames );
	return RET_SUCCESS;
}

ret_t frame_acq_run(
		const USEC deadline_us,
		camera_t* camera,
		sem_t* sem,
		bool* stop,
		acq_queue_t* acq_queue
)
{
	thread_info( "frame_acq" );
	log_verbose( "frame_acq: deadline: %06luus", deadline_us  );
	acq_entry_t acq_entry;
	while( true ) {
		if( sem_wait_nointr( sem ) ) {
			log_error( "frame_acq_run: 'sem_wait' failed: %s", strerror( errno ) );
			return RET_FAILURE;
		}
		if( (*stop) ) {
			log_info( "frame_acc: stopping\n" );
			break;
		}
		current_time = time_measure_current_time();
		timeval_t start_time = current_time;
		LOG_TIME( "START\n" );
		// return dumped frames back to camera:
		{
			log_verbose( "dumpster: read START\n" );
			pthread_mutex_lock( &frame_dumpster.mutex );
			while( frames_get_count(&frame_dumpster.frames) > 0 ) {
				frame_buffer_t* frame = frames_get( &frame_dumpster.frames );
				camera_return_frame( camera, frame );
				frames_pop( &frame_dumpster.frames );
			}
			pthread_mutex_unlock( &frame_dumpster.mutex );
			log_verbose( "dumpster: read STOP\n" );
		}
		current_time = time_measure_current_time();
		// acquire next frame:
		acq_entry.time = start_time;
		CAMERA_RUN( camera_get_frame( camera, &acq_entry.frame ));
		acq_queue_push( acq_queue, acq_entry );
		current_time = time_measure_current_time();
		timeval_t end_time = current_time;
		LOG_TIME( "END\n" );
		{
			timeval_t runtime;
			time_delta( &end_time, &start_time, &runtime );
			LOG_TIME( "RUNTIME: %4lu.%06lu\n",
					runtime.tv_sec,
					runtime.tv_nsec / 1000
			);
			USEC rt_us = time_us_from_timespec( &runtime );
			if( rt_us > deadline_us ) {
				log_error( "frame_acq: deadline failed %06luus > %06luus\n", rt_us , deadline_us );
				return RET_FAILURE;
			}
		}
	}
	return RET_SUCCESS;
}

void frame_acq_return_frame(
		frame_buffer_t frame
)
{
	log_verbose( "dumpster: add START\n" );
	pthread_mutex_lock( &frame_dumpster.mutex );
	frame_buffer_t* frame_dst = NULL;
	frames_push_start(&frame_dumpster.frames, &frame_dst);
	(*frame_dst) = frame;
	frames_push_end(&frame_dumpster.frames);
	pthread_mutex_unlock( &frame_dumpster.mutex );
	log_verbose( "dumpster: add STOP\n" );
}

int set_camera_format(
		camera_t* camera,
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval
		// const float acq_rate
)
{
	frame_interval_descrs_t interval_descrs;
	if( RET_SUCCESS != camera_list_frame_intervals(
				camera,
				pixel_format,
				size,
				&interval_descrs
	) ) {
		return RET_FAILURE;
	}
	// choose minimal supported camera frame interval:
	frame_interval_t selected_interval = { 0, 0 };
	for( uint index=0; index<interval_descrs.count; index++ ) {
		frame_interval_descr_t* interval_descr = &interval_descrs.descrs[index];
		if( 
				(selected_interval.numerator == 0 && selected_interval.denominator == 0)
				||
				((float )interval_descr->discrete.numerator / (float )interval_descr->discrete.denominator)
				< (float )selected_interval.numerator / (float )selected_interval.denominator
			) {
			selected_interval = interval_descr->discrete;
		}
	}
	const float max_interval = (float )acq_interval->numerator / (float )acq_interval->denominator;
	// const float max_interval = 1.0 / acq_rate;
	if( 
			(float )selected_interval.numerator / (float )selected_interval.denominator > max_interval
	)
	{
		log_error( "supported lowest camera interval is too long!: supported: %f, required: %f\n",
			(float )selected_interval.numerator / (float )selected_interval.denominator,
			max_interval
		);
		return RET_FAILURE;
	}
	if( RET_SUCCESS != camera_set_mode(
				camera,
				pixel_format, FORMAT_EXACT,
				size, FRAME_SIZE_EXACT,
				selected_interval, FRAME_INTERVAL_EXACT
	)) {
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

DEF_RING_BUFFER(frames,frame_buffer_t)

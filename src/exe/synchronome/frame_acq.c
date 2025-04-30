#include "frame_acq.h"

#include "exe/synchronome/acq_queue.h"
#include "lib/camera.h"
#include "lib/output.h"
#include "lib/global.h"
#include "lib/time.h"

#include <stdio.h>


int set_camera_format(
		camera_t* camera,
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval
);

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
	return RET_SUCCESS;
}

ret_t frame_acq_exit(
		camera_t* camera
)
{
	CAMERA_RUN( camera_stream_stop( camera ));
	return RET_SUCCESS;
}

ret_t frame_acq_run(
		camera_t* camera,
		sem_t* sem,
		bool* stop,
		acq_queue_t* acq_queue
)
{
	acq_entry_t acq_entry;
	timeval_t t0 = time_measure_current_time();
	while( true ) {
		if( sem_wait( sem ) ) {
			perror("sem_wait");
			return RET_FAILURE;
		}
		if( (*stop) ) {
			log_info( "frame_acc: stopping\n" );
			break;
		}
		timeval_t time = time_measure_current_time();
		timeval_t delta;
		time_delta( &time, &t0, &delta );
		acq_entry.time = delta;
		CAMERA_RUN( camera_get_frame( camera, &acq_entry.frame ));
		/*
		log_info( "frame_acc: write %lu.%lu\n",
				delta.tv_sec,
				delta.tv_nsec / 1000 / 1000
		);
		*/
		acq_queue_push( acq_queue, acq_entry );
	}
	return RET_SUCCESS;
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

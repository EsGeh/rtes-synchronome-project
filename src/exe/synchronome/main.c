#include "main.h"
// buffers:
#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"
#include "exe/synchronome/rgb_queue.h"
// services:
#include "frame_acq.h"
#include "lib/global.h"
#include "select.h"
#include "convert.h"
#include "write_to_storage.h"

#include "lib/camera.h"
#include "lib/time.h"
#include "lib/thread.h"
#include "lib/output.h"

#include <getopt.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>


/********************
 * Types
********************/

typedef struct {
	// Resources:
	camera_t camera;
	acq_queue_t acq_queue;
	select_queue_t select_queue;
	rgb_queue_t rgb_queue;
	// 
	bool stop;
	// deadlines:
	USEC deadline_select_us;
	USEC deadline_convert_us;
} runtime_data_t;

typedef struct {
	pthread_t td;
	ret_t ret;
	sem_t sem;
} camera_thread_t;

typedef struct {
	pthread_t td;
	ret_t ret;
} select_thread_t;

typedef struct {
	pthread_t td;
	ret_t ret;
} convert_thread_t;

typedef struct {
	pthread_t td;
	ret_t ret;
	// parameters:
} write_to_storage_thread_t;

typedef struct {
	float acq_interval;
	float clock_tick_interval;
	float tick_threshold;
	const int max_frames;
} select_parameters_t;

/********************
 * Global Data
********************/

static runtime_data_t data;

static camera_thread_t camera_thread;
static select_thread_t select_thread;
static convert_thread_t convert_thread;
static write_to_storage_thread_t write_to_storage_thread;

const uint select_queue_count = 16;
const uint rgb_queue_count = 64;

/********************
 * Function Decls
********************/

ret_t synchronome_init(
		const frame_size_t size,
		const uint frame_buffer_count
);
void synchronome_exit(void);

ret_t synchronome_main(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const uint frame_buffer_count,
		const frame_interval_t acq_interval,
		const frame_interval_t clock_tick_interval,
		const float tick_threshold,
		const int max_frames,
		const char* output_dir
);

void sequencer(int);

void* camera_thread_run(
		void* thread_args
);

void* select_thread_run(
		void* thread_args
);

void* convert_thread_run(
		void* thread_args
);

void* write_to_storage_thread_run(
		void* thread_args
);

/********************
 * Function Defs
********************/

#define CAMERA_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		log_error("%s\n", camera_error() ); \
		return EXIT_FAILURE; \
	} \
}

#define API_RUN( FUNC_CALL ) { \
	if( RET_SUCCESS != FUNC_CALL ) { \
		log_error("error in '%s'\n", #FUNC_CALL ); \
		return EXIT_FAILURE; \
	} \
}

ret_t synchronome_run(
		const synchronome_args_t args
)
{
	uint frame_buffer_count;
	API_RUN( select_get_frame_acc_count(
			(float )args.acq_interval.numerator / (float )args.acq_interval.denominator,
			(float )args.clock_tick_interval.numerator / (float )args.clock_tick_interval.denominator,
			&frame_buffer_count
	));
	// add safety margin:
	frame_buffer_count += 2;
	log_verbose( "frame_buffer_count: %u\n", frame_buffer_count );
	if( RET_SUCCESS != synchronome_init(
				args.size,
				frame_buffer_count
	) ) {
		synchronome_exit();
		return RET_FAILURE;
	};
	if( RET_SUCCESS != synchronome_main(
				args.pixel_format,
				args.size,
				frame_buffer_count,
				args.acq_interval,
				args.clock_tick_interval,
				args.tick_threshold,
				args.max_frames,
				args.output_dir
	) ) {
		synchronome_exit();
		return RET_FAILURE;
	};
	synchronome_exit();
	return RET_SUCCESS;
}

void synchronome_stop(void)
{
	data.stop = true;
	acq_queue_set_should_stop( &data.acq_queue );
	select_queue_set_should_stop( &data.select_queue );
	rgb_queue_set_should_stop( &data.rgb_queue );
	sem_post( &camera_thread.sem );
}

void dump_frame(frame_buffer_t frame)
{
	camera_return_frame( 
			&data.camera,
			&frame
	);
}

ret_t synchronome_init(
		const frame_size_t size,
		const uint frame_buffer_count
)
{
	camera_zero( &data.camera );
	data.stop = false;
	// allocate buffers:
	acq_queue_init(
			&data.acq_queue,
			frame_buffer_count
	);
	select_queue_init(
			&data.select_queue,
			select_queue_count
	);
	rgb_queue_init(
			&data.rgb_queue,
			size,
			rgb_queue_count
	);
	// semaphore
	if( sem_init( &camera_thread.sem, 0, 0 ) ) {
		log_error( "'sem_init': %s\n", strerror(errno) );
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

void synchronome_exit(void)
{
	if( sem_destroy ( &camera_thread.sem ) ) {
		log_error( "'sem_destroy': %s\n", strerror(errno) );
	}
	log_info( "shutdown\n" );
	rgb_queue_exit( &data.rgb_queue );
	select_queue_exit( &data.select_queue );
	acq_queue_exit( &data.acq_queue );
	camera_exit( &data.camera );
}

ret_t synchronome_main(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const uint frame_buffer_count,
		const frame_interval_t acq_interval,
		const frame_interval_t clock_tick_interval,
		const float tick_threshold,
		const int max_frames,
		const char* output_dir
)
{
	data.deadline_select_us = (float )acq_interval.numerator / (float )acq_interval.denominator * 1000 * 1000 / 2;
	data.deadline_convert_us = (float )acq_interval.numerator / (float )acq_interval.denominator * 1000 * 1000 / 2;
	if( thread_get_cpu_count() < 4 ) {
		log_error( "system provides less than 4 cpu cores" );
		return RET_FAILURE;
	}
	frame_acq_init(
			&data.camera,
			"/dev/video0",
			frame_buffer_count,
			pixel_format,
			size,
			&acq_interval
	);
	sleep(1);
	USEC capture_deadline = acq_interval.numerator * 1000 * 1000 / acq_interval.denominator;
	// loosen the constraints a bit
	// theoretically, we have to be strict.
	// But the camera seems to take some more
	// time for some frames and less on others.
	// This is ok, as long as it evens out over time.
	if( acq_interval.denominator > 1 ) {
		capture_deadline = acq_interval.numerator * 1000 * 1000 / (acq_interval.denominator-1);
	}
	capture_deadline = 1000*1000;
	API_RUN( thread_create(
			"capture",
			&camera_thread.td,
			camera_thread_run,
			&capture_deadline,
			SCHED_FIFO,
			thread_get_max_priority(SCHED_FIFO),
			1
	) );
	select_parameters_t select_params = {
		.acq_interval = (float )acq_interval.numerator / (float )acq_interval.denominator,
		.clock_tick_interval = (float )clock_tick_interval.numerator / (float )clock_tick_interval.denominator,
		.tick_threshold = tick_threshold,
		.max_frames = max_frames,
	};
	API_RUN( thread_create(
			"select",
			&select_thread.td,
			select_thread_run,
			&select_params,
			SCHED_FIFO,
			thread_get_max_priority(SCHED_FIFO),
			2
	));
	API_RUN( thread_create(
			"convert",
			&convert_thread.td,
			convert_thread_run,
			NULL,
			SCHED_FIFO,
			thread_get_max_priority(SCHED_FIFO)-1,
			2
	));
	API_RUN(thread_create(
			"storage",
			&write_to_storage_thread.td,
			write_to_storage_thread_run,
			(void* )output_dir,
			SCHED_OTHER,
			-1,
			3	
	));
	sleep(1);
	time_add_timer(
			sequencer,
			1000*1000 * acq_interval.numerator / acq_interval.denominator
	);
	ret_t ret = RET_SUCCESS;
	if( RET_SUCCESS != thread_join_ret(
				write_to_storage_thread.td
	)) {
		ret = RET_FAILURE;
	}
	if( RET_SUCCESS != thread_join_ret(
				convert_thread.td
	)) {
		ret = RET_FAILURE;
	}
	if( RET_SUCCESS != thread_join_ret(
				select_thread.td
	)) {
		ret = RET_FAILURE;
	}
	if( RET_SUCCESS != thread_join_ret(
				camera_thread.td
	)) {
		ret = RET_FAILURE;
	}
	frame_acq_exit( &data.camera );
	log_info( "done\n" );
	return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void* camera_thread_run(
		void* p
)
{
	const USEC deadline_us = *((USEC* )p);
	camera_thread.ret = frame_acq_run(
			deadline_us,
			&data.camera,
			&camera_thread.sem,
			&data.stop,
			&data.acq_queue
	);
	synchronome_stop();
	return &camera_thread.ret;
}
#pragma GCC diagnostic pop

void* select_thread_run(
		void* p
)
{
	const select_parameters_t select_params = *((select_parameters_t *)p);
	select_thread.ret = select_run(
			data.deadline_select_us,
			data.camera.format,
			select_params.acq_interval,
			select_params.clock_tick_interval,
			select_params.tick_threshold,
			select_params.max_frames,
			&data.acq_queue,
			&data.select_queue,
			dump_frame
	);
	synchronome_stop();
	return &select_thread.ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void* convert_thread_run(
		void* p
)
{
	convert_thread.ret = convert_run(
			data.deadline_convert_us,
			data.camera.format,
			&data.select_queue,
			&data.rgb_queue
	);
	synchronome_stop();
	return &convert_thread.ret;
}
#pragma GCC diagnostic pop

void* write_to_storage_thread_run(
		void* p
)
{
	char* output_dir = p;
	write_to_storage_thread.ret = write_to_storage_run(
			&data.rgb_queue,
			output_dir
	);
	synchronome_stop();
	return &write_to_storage_thread.ret;
}

void sequencer(int sig) {
	if( sig == SIGALRM ) {
		sem_post( &camera_thread.sem );
	}
}

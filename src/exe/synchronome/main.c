#include "main.h"
// buffers:
#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"
#include "exe/synchronome/rgb_queue.h"
// services:
#include "frame_acq.h"
#include "select.h"
#include "convert.h"
#include "write_to_storage.h"

#include "lib/camera.h"
#include "lib/time.h"
#include "lib/thread.h"
#include "lib/output.h"

#include <getopt.h>
#include <pthread.h>
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

/********************
 * Global Data
********************/

static runtime_data_t data;

static camera_thread_t camera_thread;
static select_thread_t select_thread;
static convert_thread_t convert_thread;
static write_to_storage_thread_t write_to_storage_thread;

const uint frame_buffer_count = 1;
const uint select_queue_count = 1;
const uint rgb_queue_count = 1;

/********************
 * Function Decls
********************/

ret_t synchronome_init(
		const frame_size_t size
);
void synchronome_exit(void);

ret_t synchronome_main(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval,
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
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval,
		const char* output_dir
)
{
	if( RET_SUCCESS != synchronome_init(
				size
	) ) {
		synchronome_exit();
		return RET_FAILURE;
	};
	if( RET_SUCCESS != synchronome_main(
				pixel_format,
				size,
				acq_interval,
				output_dir
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
		const frame_size_t size
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
		const frame_interval_t* acq_interval,
		const char* output_dir
)
{
	time_add_timer(
			sequencer,
			1000*1000 * acq_interval->numerator / acq_interval->denominator
	);
	frame_acq_init(
			&data.camera,
			"/dev/video0",
			frame_buffer_count,
			pixel_format,
			size,
			acq_interval
	);
	thread_create(
			"capture",
			&camera_thread.td,
			camera_thread_run,
			NULL
	);
	thread_create(
			"select",
			&select_thread.td,
			select_thread_run,
			NULL
	);
	thread_create(
			"convert",
			&convert_thread.td,
			convert_thread_run,
			NULL
	);
	thread_create(
			"storage",
			&write_to_storage_thread.td,
			write_to_storage_thread_run,
			(void* )output_dir
	);
	if( 0 != pthread_join(
			write_to_storage_thread.td,
			NULL
	)) {
		perror( "pthread_join" );
		return RET_FAILURE;
	}
	if( 0 != pthread_join(
			convert_thread.td,
			NULL
	)) {
		perror( "pthread_join" );
		return RET_FAILURE;
	}
	if( 0 != pthread_join(
			select_thread.td,
			NULL
	)) {
		perror( "pthread_join" );
		return RET_FAILURE;
	}
	if( 0 != pthread_join(
			camera_thread.td,
			NULL
	)) {
		perror( "pthread_join" );
		return RET_FAILURE;
	}
	if( camera_thread.ret == RET_FAILURE ) {
		return RET_FAILURE;
	}
	frame_acq_exit( &data.camera );
	log_info( "done\n" );
	return RET_SUCCESS;
}

void* camera_thread_run(
		void* p
)
{
	camera_thread.ret = frame_acq_run(
			&data.camera,
			&camera_thread.sem,
			&data.stop,
			&data.acq_queue
	);
	return &camera_thread.ret;
}

void* select_thread_run(
		void* p
)
{
	select_thread.ret = select_run(
			&data.acq_queue,
			&data.select_queue,
			dump_frame
	);
	return &select_thread.ret;
}

void* convert_thread_run(
		void* p
)
{
	convert_thread.ret = convert_run(
			data.camera.format,
			&data.select_queue,
			&data.rgb_queue,
			dump_frame
	);
	return &convert_thread.ret;
}

void* write_to_storage_thread_run(
		void* p
)
{
	char* output_dir = p;
	write_to_storage_thread.ret = write_to_storage_run(
			&data.rgb_queue,
			output_dir
	);
	return &write_to_storage_thread.ret;
}

void sequencer(int sig) {
	if( sig == SIGALRM ) {
		sem_post( &camera_thread.sem );
	}
}

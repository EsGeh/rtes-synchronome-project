#include "main.h"
#include "frame_acq.h"
#include "lib/camera.h"
#include "lib/image.h"
#include "lib/time.h"
#include "lib/thread.h"
#include "lib/output.h"

#include <getopt.h>
#include <pthread.h>
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
	camera_t camera;
	byte_t* rgb_buffer;
	uint rgb_buffer_size;
	bool stop;
} runtime_data_t;

typedef struct {
	pthread_t td;
	ret_t ret;
	sem_t sem;
	const char* output_dir;
} camera_thread_t;

/********************
 * Global Constants
********************/

static runtime_data_t data;

static camera_thread_t camera_thread;

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
	sem_post( &camera_thread.sem );
}

ret_t synchronome_init(
		const frame_size_t size
)
{
	data.rgb_buffer = NULL;
	data.rgb_buffer_size = 0;
	camera_zero( &data.camera );
	data.stop = false;
	// allocate image buffer:
	data.rgb_buffer_size = image_rgb_size(
			size.width,
			size.height
	);
	CALLOC(data.rgb_buffer, data.rgb_buffer_size, 1);
	// semaphore
	if( sem_init( &camera_thread.sem, 0, 0 ) ) {
		log_error( "'sem_init': %s", strerror(errno) );
		return RET_FAILURE;
	}
	return RET_SUCCESS;
}

void synchronome_exit(void)
{
	if( sem_destroy ( &camera_thread.sem ) ) {
		log_error( "'sem_destroy': %s", strerror(errno) );
	}
	log_info( "shutdown\n" );
	FREE( data.rgb_buffer );
	data.rgb_buffer_size = 0;
	camera_exit( &data.camera );
}

ret_t synchronome_main(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval,
		const char* output_dir
)
{
	camera_thread.output_dir = output_dir;
	const uint frame_buffer_size = 10;
	time_add_timer(
			sequencer,
			1000*1000 * acq_interval->numerator / acq_interval->denominator
	);
	frame_acq_init(
			&data.camera,
			"/dev/video0",
			frame_buffer_size,
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
			data.rgb_buffer,
			data.rgb_buffer_size,
			camera_thread.output_dir
	);
	return &camera_thread.ret;
}

void sequencer(int sig) {
	sem_post( &camera_thread.sem );
}

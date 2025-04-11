#ifndef CAMERA_H
#define CAMERA_H

#include "global.h"

#include <linux/videodev2.h>
#include <stddef.h>

#define BUFFER_SIZE 256


/********************
 * Types
********************/

typedef struct v4l2_pix_format img_format_t;

typedef struct {
	void* data;
	int index;
	size_t size;
} frame_buffer_t;

typedef struct {
	void* data;
	size_t size;
} buffer_t;

typedef struct
{
	buffer_t* buffers;
	unsigned int count;
} buffer_container_t;

// a camera is in one of these states:
// - uninitialized:
// - initialized:
//     dev_file is`open()`
// - ready:
//     frame buffers created and ready
// - capturing
typedef struct {
	char dev_name[STR_BUFFER_SIZE];
	int dev_file;
	buffer_container_t buffer_container;
} camera_t;

typedef struct {
	struct v4l2_fmtdesc format_descrs[BUFFER_SIZE];
	unsigned int count;
} format_descriptions_t;

/********************
 * Functions
********************/

// camera: uninitialized -> initialized
ret_t camera_init(
		camera_t* camera,
		const char* dev_name
);

// camera: initialized|ready -> uninitialized
ret_t camera_exit(
		camera_t* camera
);

// precondition: camera must be in state "initialized" or better
ret_t camera_list_formats(
		camera_t* camera,
		format_descriptions_t* formats
);

// precondition: camera must be in state "initialized"
ret_t camera_set_format(
		camera_t* camera,
		const unsigned int min_width,
		const unsigned int min_height,
		const __u32* requested_format,
		const bool format_required
);

// camera: initialized -> ready
ret_t camera_init_buffer(
		camera_t* camera,
		const unsigned int buffer_count
);

// camera: ready -> capturing
ret_t camera_stream_start(
		camera_t* camera
);

// camera: capturing -> ready
ret_t camera_stream_stop(
		camera_t* camera
);

// precondition: camera must be in state "capturing"
ret_t camera_get_frame(
		camera_t* camera,
		frame_buffer_t* buffer
);

// precondition: camera must be in state "capturing"
ret_t camera_return_frame(
		camera_t* camera,
		frame_buffer_t* buffer
);

char* camera_error();
void camera_reset_error();

#endif

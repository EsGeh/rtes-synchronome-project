/****************************
 * High Level Camera Access
 *
 * *REMARK*:
 * handling of 
 ***************************/
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

typedef enum {
	FORMAT_ANY = 0,
	FORMAT_EXACT = 1
} format_constraint_t;

typedef enum {
	FRAME_SIZE_ANY = 0,
	FRAME_SIZE_EXACT = 1
} frame_size_constraint_t;

typedef enum {
	FRAME_INTERVAL_ANY = 0,
	FRAME_INTERVAL_EXACT,
	FRAME_INTERVAL_QUERY
} frame_interval_constraint_t;

// types for describing/specifying camera modes:

typedef struct v4l2_fmtdesc pixel_format_descr_t;
typedef struct v4l2_frmsizeenum frame_size_descr_t;
typedef struct v4l2_frmivalenum frame_interval_descr_t;

typedef __u32 pixel_format_t;
typedef struct {
	uint width;
	uint height;
} frame_size_t;
typedef struct v4l2_fract frame_interval_t;

typedef struct {
	pixel_format_descr_t pixel_format_descr;
	frame_size_descr_t frame_size_descr;
	frame_interval_descr_t frame_interval_descr;
} camera_mode_descr_t;

typedef struct {
	pixel_format_t pixel_format;
	frame_size_t frame_size;
	frame_interval_t frame_interval;
} camera_mode_t;

// aggregate types:

typedef struct {
	unsigned int count;
	pixel_format_descr_t format_descrs[BUFFER_SIZE];
} format_descriptions_t;

typedef struct {
	unsigned int count;
	frame_size_descr_t frame_size_descrs[BUFFER_SIZE];
} size_descrs_t;

typedef struct {
	unsigned int count;
	frame_interval_descr_t descrs[BUFFER_SIZE];
} frame_interval_descrs_t;

typedef struct {
	uint count;
	camera_mode_descr_t mode_descrs[BUFFER_SIZE];
} camera_mode_descrs_t;

typedef struct {
	uint count;
	camera_mode_t modes[BUFFER_SIZE];
} camera_modes_t;

// frames, framebuffers...:

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
	img_format_t format;
	frame_interval_t frame_interval;
	// keep track of owned frames
	// (useful for debugging)
	uint currently_owned_frames;
} camera_t;

/********************
 * Functions
********************/

void camera_zero( camera_t* camera );

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

// precondition: camera must be in state "initialized" or better
ret_t camera_list_sizes(
		camera_t* camera,
		const pixel_format_t pixel_format,
		size_descrs_t* frame_size_descrs
);

// precondition: camera must be in state "initialized" or better
ret_t camera_list_frame_intervals(
		camera_t* camera,
		const pixel_format_t pixel_format,
		const frame_size_t frame_size,
		frame_interval_descrs_t* frame_interval_descrs
);

// precondition: camera must be in state "initialized" or better
ret_t camera_list_modes(
		camera_t* camera,
		camera_mode_descrs_t* modes
);

// precondition: camera must be in state "initialized"
ret_t camera_set_mode(
		camera_t* camera,
		const pixel_format_t requested_format,
		const format_constraint_t format_constraint,
		const frame_size_t frame_size,
		const frame_size_constraint_t frame_size_constraint,
		const frame_interval_t frame_interval,
		const frame_interval_constraint_t frame_interval_constraint
);

// precondition: camera must be in state "initialized"
ret_t camera_get_mode(
		camera_t* camera,
		camera_mode_t* mode
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

bool camera_mode_descr_equal(
		const camera_mode_descr_t* mode1,
		const camera_mode_descr_t* mode2
);

#endif

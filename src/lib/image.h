/****************************
 * Image Data Processing
 * Utilities
 *
 * *REMARK*:
 * Handling of image formats is experimental/incomplete (ie: colorspaces)
 *
 * see: https://www.kernel.org/doc/html/v4.9/media/uapi/v4l/pixfmt.html 
 ***************************/
#ifndef IMAGE_H
#define IMAGE_H

#include "global.h"

#include <stddef.h>

#include <linux/videodev2.h>


/********************
 * Types
********************/

typedef struct v4l2_pix_format img_format_t;

/********************
 * Functions
********************/

ret_t image_save_ppm(
		const char* filename,
		const char* comment,
		const void* buffer,
		const uint buffer_size,
		const uint width,
		const uint height
);

ret_t image_save_ppm_to_ram(
		const char* filename,
		const char* comment,
		const void* buffer,
		const uint buffer_size,
		const uint width,
		const uint height,
		FILE* file
);

ret_t image_convert_to_rgb(
		const img_format_t src_format,
		const void* src_buffer,
		const void* dst_buffer,
		const size_t dst_size
);

ret_t image_diff(
		const img_format_t src_format,
		const void* src_buffer_1,
		const void* src_buffer_2,
		float* result
);

size_t image_rgb_size(
		const uint width,
		const uint height
);

#endif

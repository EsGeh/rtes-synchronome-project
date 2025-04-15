#ifndef IMAGE_H
#define IMAGE_H

#include "global.h"

#include <stddef.h>

#include <linux/videodev2.h>

typedef struct v4l2_pix_format img_format_t;

/********************
 * Functions
********************/

ret_t image_save_ppm(
		const char* comment,
		const void* buffer,
		const img_format_t format,
		const char* filename
);

size_t rgb_size(
		const img_format_t src_format
);

ret_t image_convert_to_rgb(
		const img_format_t src_format,
		const void* src_buffer,
		const void* dst_buffer,
		const size_t dst_size
);

#endif

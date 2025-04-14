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
		const size_t size,
		const img_format_t format,
		const char* filename
);

#endif

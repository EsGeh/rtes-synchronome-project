#pragma once

#include "lib/global.h"

#include "lib/camera.h"

/********************
 * Function Decls
********************/

typedef struct {
	pixel_format_t pixel_format;
	frame_size_t size;
	frame_interval_t acq_interval;
	frame_interval_t clock_tick_interval;
	float tick_threshold;
	uint max_frames;
	char* output_dir;
	uint compress_bundle_size; // 0 means no bundling
} synchronome_args_t;

ret_t synchronome_run( const synchronome_args_t args );

void synchronome_stop(void);

#pragma once

#include "lib/global.h"

#include "lib/camera.h"

/********************
 * Function Decls
********************/

typedef struct {
	frame_size_t size;
	frame_interval_t acq_interval;
	frame_interval_t clock_tick_interval;
	bool save_all;
	char* output_dir;
} synchronome_args_t;

ret_t synchronome_run(
		const pixel_format_t pixel_format,
		const frame_size_t size,
		const frame_interval_t acq_interval,
		const frame_interval_t clock_tick_interval,
		const char* output_dir,
		const bool save_all
);

void synchronome_stop(void);

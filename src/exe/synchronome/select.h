#pragma once

#include "acq_queue.h"
#include "select_queue.h"

#include <semaphore.h>

ret_t select_run(
		const USEC deadline_us,
		const img_format_t src_format,
		const float acq_interval,
		const float clock_tick_interval,
		const float tick_threshold,
		const int max_frames, // -1 means no limit
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
);

// how many frames will be accumulated
// at maximum while running `select_run`?
ret_t select_get_frame_acc_count(
		const float acq_interval,
		const float clock_tick_interval,
		uint* frame_acc_count
);

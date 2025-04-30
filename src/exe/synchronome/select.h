#pragma once

#include "acq_queue.h"
#include "select_queue.h"

#include <semaphore.h>

ret_t select_run(
		const img_format_t src_format,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
);

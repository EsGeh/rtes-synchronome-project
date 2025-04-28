#pragma once

#include "select_queue.h"
#include "rgb_queue.h"

#include <semaphore.h>

ret_t convert_run(
		const img_format_t src_format,
		select_queue_t* input_queue,
		rgb_queue_t* rgb_queue,
		dump_frame_func_t dump_frame
);

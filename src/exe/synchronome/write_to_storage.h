#pragma once

#include "rgb_queue.h"

#include <semaphore.h>

ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		const frame_size_t frame_size,
		const char* output_dir
);

#pragma once

#include "rgb_queue.h"

#include <semaphore.h>

ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		const char* output_dir
);

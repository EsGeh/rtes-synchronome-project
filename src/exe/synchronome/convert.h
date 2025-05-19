#pragma once

#include "queues/select_queue.h"
#include "queues/rgb_queue.h"

#include <semaphore.h>

ret_t convert_run(
		const USEC deadline_us,
		const img_format_t src_format,
		select_queue_t* input_queue,
		rgb_queue_t* rgb_queue
);

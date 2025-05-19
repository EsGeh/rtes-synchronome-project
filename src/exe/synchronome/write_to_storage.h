#pragma once

#include "queues/rgb_queue.h"

#include <semaphore.h>

ret_t write_to_storage_run(
		rgb_queue_t* rgb_queue,
		rgb_consumers_queue_t* rgb_consumers_queue,
		sem_t* other_services_done,
		const frame_size_t frame_size,
		const char* output_dir
);

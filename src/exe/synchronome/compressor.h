#pragma once

#include "lib/image.h"
#include "queues/rgb_queue.h"
#include "lib/global.h"

#include <semaphore.h>


typedef struct {
	uint package_size;
	char* shared_dir;
	frame_size_t image_size;
} compressor_args_t;


ret_t compressor_run(
		const compressor_args_t args,
		rgb_consumers_queue_t* input_queue,
		sem_t* done_processing
);

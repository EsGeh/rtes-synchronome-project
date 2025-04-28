#pragma once

#include "lib/camera.h"
#include "acq_queue.h"

#include <semaphore.h>

ret_t frame_acq_init(
		camera_t* camera,
		const char* dev_name,
		const uint buffer_size,
		const pixel_format_t required_format,
		const frame_size_t size,
		const frame_interval_t* acq_interval
);

ret_t frame_acq_exit(
		camera_t* camera
);

ret_t frame_acq_run(
		camera_t* camera,
		sem_t* sem,
		bool* stop,
		acq_queue_t* acq_queue
);

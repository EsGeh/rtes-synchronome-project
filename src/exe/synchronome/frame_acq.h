#pragma once

#include "lib/camera.h"
#include "queues/acq_queue.h"

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
		const USEC deadline_us,
		camera_t* camera,
		sem_t* sem,
		bool* stop,
		acq_queue_t* acq_queue
);

// this never blocks, but
// simply enqueues the frame to
// be returned to the camera
void frame_acq_return_frame(
		frame_buffer_t frame
);

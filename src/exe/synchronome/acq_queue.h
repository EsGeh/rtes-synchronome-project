#pragma once

#include "lib/camera.h"
#include "lib/spsc_queue.h"
#include "lib/time.h"

typedef void (*dump_frame_func_t)(frame_buffer_t frame);

typedef struct {
	timeval_t time;
	frame_buffer_t frame;
} acq_entry_t;

DECL_SPSC_QUEUE(acq_queue,acq_entry_t)

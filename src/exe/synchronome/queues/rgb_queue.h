#pragma once

#include "lib/camera.h"
#include "lib/time.h"
#include "lib/spsc_queue.h"

#include <semaphore.h>

typedef struct {
	byte_t* data;
	uint size;
} rgb_frame_t;

typedef struct {
	timeval_t time;
	rgb_frame_t frame;
} rgb_entry_t;

DECL_SPSC_QUEUE(rgb_queue,rgb_entry_t);

void rgb_queue_init_frames( rgb_queue_t* queue, const frame_size_t size);
void rgb_queue_exit_frames( rgb_queue_t* queue );

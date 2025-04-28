#pragma once

#include "lib/camera.h"
#include "lib/time.h"

#include <semaphore.h>

typedef struct {
	byte_t* data;
	uint size;
} rgb_frame_t;

typedef struct {
	timeval_t time;
	rgb_frame_t frame;
} rgb_entry_t;

typedef struct {
	rgb_entry_t* entries;
	uint max_count;
	uint read_pos;
	uint write_pos;
	sem_t read_sem;
	sem_t write_sem;
	bool stop;
	frame_size_t frame_size;
} rgb_queue_t;

void rgb_queue_init(
		rgb_queue_t* queue,
		const frame_size_t size,
		uint max_count
);
void rgb_queue_exit(rgb_queue_t* queue);

uint rgb_queue_get_max_count(
		rgb_queue_t* queue
);

bool rgb_queue_get_should_stop(
		rgb_queue_t* queue
);

void rgb_queue_set_should_stop(
		rgb_queue_t* queue
);

frame_size_t rgb_queue_get_frame_size(
		rgb_queue_t* queue
);

// READ:

void rgb_queue_read_start(
		rgb_queue_t* queue
);

void rgb_queue_read_get(
		rgb_queue_t* queue,
		rgb_entry_t** entry
);

void rgb_queue_read_stop_dump(
		rgb_queue_t* queue
);

// WRITE:

void rgb_queue_push_start(
		rgb_queue_t* queue,
		rgb_entry_t** entry
);

void rgb_queue_push_end(
		rgb_queue_t* queue
);

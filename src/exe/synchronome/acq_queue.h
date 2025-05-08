#pragma once

#include "lib/camera.h"
#include "lib/time.h"

#include <semaphore.h>

typedef void (*dump_frame_func_t)(frame_buffer_t frame);

typedef struct {
	timeval_t time;
	frame_buffer_t frame;
} acq_entry_t;

typedef struct {
	acq_entry_t* entries;
	uint max_count;
	uint read_pos;
	uint write_pos;
	sem_t read_sem;
	sem_t write_sem;
	bool stop;
} acq_queue_t;

void acq_queue_init(
		acq_queue_t* queue,
		uint max_count
);
void acq_queue_exit(acq_queue_t* queue);

uint acq_queue_get_max_count(
		const acq_queue_t* queue
);

bool acq_queue_get_should_stop(
		const acq_queue_t* queue
);

void acq_queue_set_should_stop(
		acq_queue_t* queue
);

// READ:

void acq_queue_read_start(
		acq_queue_t* queue
);

acq_entry_t* acq_queue_read_get(
		const acq_queue_t* queue,
		const uint index
);

void acq_queue_read_stop_dump(
		acq_queue_t* queue
);

// WRITE:

void acq_queue_push(
		acq_queue_t* queue,
		acq_entry_t entry
);

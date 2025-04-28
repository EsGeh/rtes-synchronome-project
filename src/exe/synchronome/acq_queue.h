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
	uint count;
	uint read_pos;
	sem_t read_sem;
	bool stop;
	pthread_mutex_t mutex;
} acq_queue_t;

void acq_queue_init(
		acq_queue_t* queue,
		uint max_count
);
void acq_queue_exit(acq_queue_t* queue);

uint acq_queue_get_max_count(
		acq_queue_t* queue
);

uint acq_queue_get_count(
		acq_queue_t* queue
);

bool acq_queue_is_full(
		acq_queue_t* queue
);

void acq_queue_wait(
		acq_queue_t* queue
);

bool acq_queue_should_stop(
		acq_queue_t* queue
);

void acq_queue_stop(
		acq_queue_t* queue
);

void acq_queue_push(
		acq_queue_t* queue,
		acq_entry_t entry
);

void acq_queue_peek(
		acq_queue_t* queue,
		acq_entry_t* entry
);

void acq_queue_pop(
		acq_queue_t* queue
);

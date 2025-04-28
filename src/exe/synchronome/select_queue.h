#pragma once

#include "exe/synchronome/acq_queue.h"

#include <semaphore.h>

/*
typedef struct {
	timeval_t time;
	frame_buffer_t frame;
} select_entry_t;
*/

typedef acq_entry_t select_entry_t;

typedef struct {
	select_entry_t* entries;
	uint max_count;
	uint count;
	uint read_pos;
	sem_t read_sem;
	bool stop;
	pthread_mutex_t mutex;
	/*
	sem_t write_sem;
	*/
} select_queue_t;

void select_queue_init(
		select_queue_t* queue,
		uint max_count
);
void select_queue_exit(select_queue_t* queue);

uint select_queue_get_max_count(
		select_queue_t* queue
);

uint select_queue_get_count(
		select_queue_t* queue
);

bool select_queue_is_full(
		select_queue_t* queue
);

void select_queue_wait(
		select_queue_t* queue
);

bool select_queue_should_stop(
		select_queue_t* queue
);

void select_queue_stop(
		select_queue_t* queue
);

void select_queue_push(
		select_queue_t* queue,
		select_entry_t entry
);

void select_queue_peek(
		select_queue_t* queue,
		select_entry_t* entry
);

void select_queue_pop(
		select_queue_t* queue
);

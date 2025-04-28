#pragma once

#include "exe/synchronome/acq_queue.h"

#include <semaphore.h>


typedef acq_entry_t select_entry_t;

typedef struct {
	select_entry_t* entries;
	uint max_count;
	uint read_pos;
	uint write_pos;
	sem_t read_sem;
	sem_t write_sem;
	bool stop;
} select_queue_t;

void select_queue_init(
		select_queue_t* queue,
		uint max_count
);
void select_queue_exit(select_queue_t* queue);

uint select_queue_get_max_count(
		select_queue_t* queue
);

bool select_queue_get_should_stop(
		select_queue_t* queue
);

void select_queue_set_should_stop(
		select_queue_t* queue
);

// READ:

void select_queue_read_start(
		select_queue_t* queue
);

void select_queue_read_get(
		select_queue_t* queue,
		select_entry_t* entry
);

void select_queue_read_stop_dump(
		select_queue_t* queue
);

// WRITE:

void select_queue_push(
		select_queue_t* queue,
		select_entry_t entry
);

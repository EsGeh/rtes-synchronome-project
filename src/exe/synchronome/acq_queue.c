#include "acq_queue.h"

#include "lib/output.h"
#include "lib/semaphore.h"

#include <pthread.h>
#include <semaphore.h>
#include <string.h>


void acq_queue_init(
		acq_queue_t* queue,
		uint max_count
)
{
	queue->max_count = max_count;
	queue->entries = NULL;
	CALLOC( queue->entries, queue->max_count, sizeof(acq_entry_t));
	queue->read_pos = 0;
	queue->write_pos = 0;
	queue->count = 0;
	if( 0 != sem_init( &queue->read_sem, 0, 0 ) ) {
		log_error( "'sem_init' failed!" );
		exit(EXIT_FAILURE);
	}
	if( 0 != sem_init( &queue->write_sem, 0, max_count ) ) {
		log_error( "'sem_init' failed!" );
		exit(EXIT_FAILURE);
	}
	queue->stop = false;
}

void acq_queue_exit(acq_queue_t* queue)
{
	if( 0 != sem_destroy( &queue->write_sem ) ) {
		log_error( "'sem_destroy' failed!" );
		exit(EXIT_FAILURE);
	}
	if( 0 != sem_destroy( &queue->read_sem ) ) {
		log_error( "'sem_destroy' failed!" );
		exit(EXIT_FAILURE);
	}
	FREE( queue->entries );
	queue->max_count = 0;
}

uint acq_queue_get_max_count(
		const acq_queue_t* queue
)
{
	return queue->max_count;
}

bool acq_queue_get_should_stop(
		const acq_queue_t* queue
)
{
	return queue->stop;
}

void acq_queue_set_should_stop(
		acq_queue_t* queue
)
{
	queue->stop = true;
	if( sem_post( &queue->read_sem ) ) {
		log_error( "acq_queue_set_should_stop: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
}

// READ:

void acq_queue_read_start(
		acq_queue_t* queue
)
{
	if( sem_wait_nointr( &queue->read_sem ) ) {
		log_error( "acq_queue_read_start: 'sem_wait' failed: %s", strerror( errno ) );
		exit(1);
	}
}

acq_entry_t* acq_queue_read_get(
		const acq_queue_t* queue,
		const uint index
)
{
	return &queue->entries[
		(queue->read_pos + index) % queue->max_count
	];
}

void acq_queue_read_stop_dump(
		acq_queue_t* queue
)
{
	queue->read_pos = (queue->read_pos + 1) % queue->max_count;
	if( sem_post( &queue->write_sem ) == -1 ) {
		log_error( "acq_queue_read_stop_dump: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
	queue->count--;
	log_verbose( "acq_queue: %d/%d", queue->count, queue->max_count);
}

// WRITE:

void acq_queue_push(
		acq_queue_t* queue,
		acq_entry_t entry
)
{
	if( sem_wait( &queue->write_sem ) ) {
		log_error( "acq_queue_push: 'sem_wait' failed: %s", strerror( errno ) );
		exit(1);
	}
	queue->count++;
	log_verbose( "acq_queue: %d/%d", queue->count, queue->max_count);
	queue->entries[queue->write_pos] = entry;
	queue->write_pos = (queue->write_pos+1) % queue->max_count;
	if( sem_post( &queue->read_sem ) == -1 ) {
		log_error( "acq_queue_push: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
}

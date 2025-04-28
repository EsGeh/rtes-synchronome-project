#include "acq_queue.h"

#include "lib/output.h"

#include <pthread.h>
#include <semaphore.h>


void acq_queue_init(
		acq_queue_t* queue,
		uint max_count
)
{
	queue->max_count = max_count;
	queue->entries = NULL;
	CALLOC( queue->entries, queue->max_count, sizeof(acq_entry_t));
	queue->count = 0;
	queue->read_pos = 0;
	if( 0 != sem_init( &queue->read_sem, 0, 0 ) ) {
		log_error( "'sem_init' failed!" );
		exit(EXIT_FAILURE);
	}
	queue->stop = false;
	sem_init( &queue->read_sem, 0, 0);
	if( 0 != pthread_mutex_init( &queue->mutex, NULL )) {
		log_error( "'pthread_mutex_init' failed!" );
		exit(EXIT_FAILURE);
	}
}

void acq_queue_exit(acq_queue_t* queue)
{
	if( 0 != sem_destroy( &queue->read_sem ) ) {
		log_error( "'sem_destroy' failed!" );
		exit(EXIT_FAILURE);
	}
	if( 0 != pthread_mutex_destroy( &queue->mutex )) {
		log_error( "'pthread_mutex_destroy' failed!" );
		exit(EXIT_FAILURE);
	}
	FREE( queue->entries );
	queue->max_count = 0;
	sem_destroy( &queue->read_sem);
}

uint acq_queue_get_max_count(
		acq_queue_t* queue
)
{
	return queue->max_count;
}

uint acq_queue_get_count(
		acq_queue_t* queue
)
{
	return queue->count;
}

bool acq_queue_is_full(
		acq_queue_t* queue
)
{
	pthread_mutex_lock( &queue->mutex );
	bool ret = queue->count >= queue->max_count;
	pthread_mutex_unlock( &queue->mutex );
	return ret;
}

void acq_queue_wait(
		acq_queue_t* queue
)
{
	sem_wait( &queue->read_sem );
}

bool acq_queue_should_stop(
		acq_queue_t* queue
)
{
	return queue->stop;
}

void acq_queue_stop(
		acq_queue_t* queue
)
{
	queue->stop = true;
	sem_post( &queue->read_sem );
}

void acq_queue_push(
		acq_queue_t* queue,
		acq_entry_t entry
)
{
	pthread_mutex_lock( &queue->mutex );
	queue->entries[
		(queue->read_pos + queue->count) % queue->max_count
	] = entry;
	queue->count = queue->count + 1;
	pthread_mutex_unlock( &queue->mutex );
	sem_post( &queue->read_sem );
}

void acq_queue_pop(
		acq_queue_t* queue
)
{
	pthread_mutex_lock( &queue->mutex );
	queue->read_pos = (queue->read_pos + 1) % queue->max_count;
	queue->count--;
	pthread_mutex_unlock( &queue->mutex );
}

void acq_queue_peek(
		acq_queue_t* queue,
		acq_entry_t* entry
)
{
	pthread_mutex_lock( &queue->mutex );
	*entry = queue->entries[queue->read_pos];
	pthread_mutex_unlock( &queue->mutex );
}

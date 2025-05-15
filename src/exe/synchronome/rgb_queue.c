#include "rgb_queue.h"
#include "lib/image.h"
#include "lib/semaphore.h"
#include "lib/output.h"

#include <pthread.h>
#include <semaphore.h>
#include <string.h>


void rgb_queue_init(
		rgb_queue_t* queue,
		const frame_size_t size,
		uint max_count
)
{
	queue->max_count = max_count;
	queue->entries = NULL;
	CALLOC( queue->entries, queue->max_count, sizeof(rgb_entry_t));
	queue->read_pos = 0;
	queue->write_pos = 0;
	queue->count = 0;
	for( uint i=0; i<max_count; ++i ) {
		queue->entries[i].frame.data = NULL;
		queue->entries[i].frame.size = image_rgb_size( size.width, size.height );
		CALLOC(
				queue->entries[i].frame.data,
				queue->entries[i].frame.size,
				1
		);
	}
	if( 0 != sem_init( &queue->read_sem, 0, 0 ) ) {
		log_error( "'sem_init' failed!" );
		exit(EXIT_FAILURE);
	}
	if( 0 != sem_init( &queue->write_sem, 0, max_count ) ) {
		log_error( "'sem_init' failed!" );
		exit(EXIT_FAILURE);
	}
	queue->stop = false;
	queue->frame_size = size;
}

void rgb_queue_exit(rgb_queue_t* queue)
{
	for( uint i=0; i<queue->max_count; ++i ) {
		FREE( queue->entries[i].frame.data );
	}
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
	sem_destroy( &queue->read_sem);
}

uint rgb_queue_get_max_count(
		rgb_queue_t* queue
)
{
	return queue->max_count;
}

frame_size_t rgb_queue_get_frame_size(
		rgb_queue_t* queue
)
{
	return queue->frame_size;
}

bool rgb_queue_get_should_stop(
		rgb_queue_t* queue
)
{
	return queue->stop;
}

void rgb_queue_set_should_stop(
		rgb_queue_t* queue
)
{
	queue->stop = true;
	if( sem_post( &queue->read_sem ) == -1 ) {
		log_error( "rgb_queue_set_should_stop: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
}

// READ:

void rgb_queue_read_start(
		rgb_queue_t* queue
)
{
	if( sem_wait_nointr(&queue->read_sem) ) {
		log_error( "rgb_queue_read_start: 'sem_wait' failed: %s", strerror( errno ) );
		exit(1);
	}
}

void rgb_queue_read_get(
		rgb_queue_t* queue,
		rgb_entry_t** entry
)
{
	*entry = &queue->entries[queue->read_pos];
}

void rgb_queue_read_stop_dump(
		rgb_queue_t* queue
)
{
	queue->read_pos = (queue->read_pos + 1) % queue->max_count;
	if( sem_post(&queue->write_sem) ) {
		log_error( "rgb_queue_read_stop_dump: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
	queue->count--;
	log_verbose( "rgb_queue: %d/%d", queue->count, queue->max_count);
}

// WRITE:

void rgb_queue_push_start(
		rgb_queue_t* queue,
		rgb_entry_t** entry
)
{
	if( sem_wait_nointr( &queue->write_sem ) ) {
		log_error( "rgb_queue_push_start: 'sem_wait' failed: %s", strerror( errno ) );
		exit(1);
	}
	queue->count++;
	log_verbose( "rgb_queue: %d/%d", queue->count, queue->max_count);
	(*entry) = &queue->entries[queue->write_pos];
}

void rgb_queue_push_end(
		rgb_queue_t* queue
)
{
	queue->write_pos = (queue->write_pos+1) % queue->max_count;
	if( sem_post( &queue->read_sem ) ) {
		log_error( "rgb_queue_push_end: 'sem_post' failed: %s", strerror( errno ) );
		exit(1);
	}
}

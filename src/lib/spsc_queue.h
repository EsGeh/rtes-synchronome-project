/****************************
 * Single Producer -
 * Single Consumer -
 * Blocking - Queue
 ***************************/
#pragma once

#include "global.h"
#include "semaphore.h"

#include <string.h>


#define DECL_SPSC_QUEUE(NAME,ENTRY_T, DBG_LOG,ERR_LOG) \
\
typedef struct { \
	ENTRY_T* entries; \
	uint max_count; \
	uint read_pos; \
	uint write_pos; \
	sem_t read_sem; \
	sem_t write_sem; \
	bool stop; \
	_Atomic int count; \
} NAME##_t; \
 \
void NAME##_init( \
		NAME##_t* queue, \
		const uint max_count \
); \
void NAME##_exit( \
		NAME##_t* queue \
); \
 \
uint NAME##_get_max_count( \
		NAME##_t* queue \
); \
 \
uint NAME##_get_count( \
		NAME##_t* queue \
); \
 \
bool NAME##_get_should_stop( \
		const NAME##_t* queue \
); \
 \
void NAME##_set_should_stop( \
		NAME##_t* queue \
); \
 \
void NAME##_read_start( \
		NAME##_t* queue \
); \
 \
ENTRY_T* NAME##_read_get( \
		NAME##_t* queue \
); \
ENTRY_T* NAME##_read_get_index( \
		NAME##_t* queue, \
		const uint index \
); \
 \
void NAME##_read_stop_dump( \
		NAME##_t* queue \
); \
 \
void NAME##_push_start( \
		NAME##_t* queue, \
		ENTRY_T** entry \
); \
 \
void NAME##_push_end( \
		NAME##_t* queue \
);

/*****************
 * Definitions
 *****************/

#define DEF_SPSC_QUEUE(NAME,ENTRY_T,DBG_LOG,ERR_LOG) \
 \
void NAME##_init( \
		NAME##_t* queue, \
		const uint max_count \
) \
{ \
	queue->max_count = max_count; \
	queue->entries = NULL; \
	CALLOC( queue->entries, queue->max_count, sizeof(ENTRY_T) ); \
	queue->read_pos = 0; \
	queue->write_pos = 0; \
	queue->count = 0; \
	if( 0 != sem_init( &queue->read_sem, 0, 0 ) ) { \
		ERR_LOG( "%s_t: 'sem_init' failed: %s\n", #NAME, strerror( errno )); \
	} \
	if( 0 != sem_init( &queue->write_sem, 0, max_count ) ) { \
		ERR_LOG( "%s_t: 'sem_init' failed: %s\n", #NAME, strerror(errno) ); \
	} \
	queue->stop = false; \
} \
 \
void NAME##_exit( \
		NAME##_t* queue \
) \
{ \
	if( 0 != sem_destroy( &queue->write_sem ) ) { \
		ERR_LOG( "%s_t: 'sem_destroy' failed: %s\n", #NAME, strerror(errno) ); \
	} \
	if( 0 != sem_destroy( &queue->read_sem ) ) { \
		ERR_LOG( "%s_t: 'sem_destroy' failed: %s\n", #NAME, strerror(errno) ); \
	} \
	FREE( queue->entries ); \
	queue->max_count = 0; \
} \
 \
uint NAME##_get_max_count( \
		NAME##_t* queue \
) \
{ \
	return queue->max_count; \
} \
 \
uint NAME##_get_count( \
		NAME##_t* queue \
) \
{ \
	return queue->count; \
} \
 \
bool NAME##_get_should_stop( \
		const NAME##_t* queue \
) \
{ \
	return queue->stop; \
} \
 \
void NAME##_set_should_stop( \
		NAME##_t* queue \
) \
{ \
	queue->stop = true; \
	if( sem_post( &queue->read_sem ) ) { \
		ERR_LOG( "%s_set_should_stop: 'sem_post' failed: %s\n", #NAME, strerror( errno ) ); \
	} \
} \
 \
void NAME##_read_start( \
		NAME##_t* queue \
) \
{ \
	if( sem_wait_nointr( &queue->read_sem ) ) { \
		ERR_LOG( "%s_read_start: 'sem_wait' failed: %s\n!", #NAME, strerror( errno ) ); \
	} \
} \
 \
ENTRY_T* NAME##_read_get( \
		NAME##_t* queue \
) \
{ \
	return NAME##_read_get_index(queue, 0); \
} \
 \
ENTRY_T* NAME##_read_get_index( \
		NAME##_t* queue, \
		const uint index \
) \
{ \
	return &queue->entries[ \
		(queue->read_pos+index) % queue->max_count \
	]; \
} \
 \
void NAME##_read_stop_dump( \
		NAME##_t* queue \
) \
{ \
	queue->read_pos = (queue->read_pos + 1) % queue->max_count; \
	if( sem_post( &queue->write_sem ) ) { \
		ERR_LOG( "%s_read_stop_dump: 'sem_post' failed: %s\n", #NAME, strerror( errno ) ); \
	} \
	queue->count--; \
	DBG_LOG( "%s_t: %d/%d\n", #NAME, queue->count, queue->max_count); \
} \
 \
void NAME##_push_start( \
		NAME##_t* queue, \
		ENTRY_T** entry \
) \
{ \
	if( sem_wait( &queue->write_sem ) ) { \
		ERR_LOG( "%s_push_start: 'sem_wait' failed: %s\n", #NAME, strerror( errno ) ); \
	} \
	queue->count++; \
	DBG_LOG( "%s_t: %d/%d\n", #NAME, queue->count, queue->max_count); \
	(*entry) = &queue->entries[queue->write_pos]; \
} \
 \
void NAME##_push_end( \
		NAME##_t* queue \
) \
{ \
	queue->write_pos = (queue->write_pos+1) % queue->max_count; \
	if( sem_post( &queue->read_sem ) ) { \
		ERR_LOG( "%s_push_end: 'sem_post' failed: %s\n", #NAME, strerror( errno ) ); \
	} \
}

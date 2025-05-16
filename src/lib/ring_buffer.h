/****************************
 * Ring Buffer (no synchronization)
 ***************************/
#pragma once

#include "global.h"

#define DECL_RING_BUFFER(NAME,ENTRY_T) \
\
typedef struct { \
	ENTRY_T* entries; \
	uint max_count; \
	uint read_pos; \
	uint write_pos; \
	uint count; \
} NAME##_t; \
 \
void NAME##_init( \
		NAME##_t* buffer, \
		const uint max_count \
); \
void NAME##_exit( \
		NAME##_t* buffer \
); \
 \
uint NAME##_get_max_count( \
		NAME##_t* buffer \
); \
 \
uint NAME##_get_count( \
		NAME##_t* buffer \
); \
 \
ENTRY_T* NAME##_get( \
		NAME##_t* buffer \
); \
 \
ENTRY_T* NAME##_get_index( \
		NAME##_t* buffer, \
		const uint index \
); \
 \
void NAME##_pop( \
		NAME##_t* buffer \
); \
 \
void NAME##_push_start( \
		NAME##_t* buffer, \
		ENTRY_T** entry \
); \
 \
void NAME##_push_end( \
		NAME##_t* buffer \
);

/*****************
 * Definitions
 *****************/

#define DEF_RING_BUFFER(NAME,ENTRY_T) \
 \
void NAME##_init( \
		NAME##_t* buffer, \
		const uint max_count \
) \
{ \
	buffer->max_count = max_count; \
	buffer->entries = NULL; \
	CALLOC( buffer->entries, buffer->max_count, sizeof(ENTRY_T) ); \
	buffer->read_pos = 0; \
	buffer->write_pos = 0; \
	buffer->count = 0; \
} \
 \
void NAME##_exit( \
		NAME##_t* buffer \
) \
{ \
	FREE( buffer->entries ); \
	buffer->max_count = 0; \
} \
 \
uint NAME##_get_max_count( \
		NAME##_t* buffer \
) \
{ \
	return buffer->max_count; \
} \
 \
uint NAME##_get_count( \
		NAME##_t* buffer \
) \
{ \
	return buffer->count; \
} \
 \
ENTRY_T* NAME##_get( \
		NAME##_t* buffer \
) \
{ \
	return NAME##_get_index(buffer, 0); \
} \
 \
ENTRY_T* NAME##_get_index( \
		NAME##_t* buffer, \
		const uint index \
) \
{ \
	return &buffer->entries[ \
		(buffer->read_pos+index) % buffer->max_count \
	]; \
} \
 \
void NAME##_pop( \
		NAME##_t* buffer \
) \
{ \
	assert( buffer->count > 0 ); \
	buffer->read_pos = (buffer->read_pos + 1) % buffer->max_count; \
	buffer->count--; \
} \
 \
 \
void NAME##_push_start( \
		NAME##_t* buffer, \
		ENTRY_T** entry \
) \
{ \
	assert( buffer->count < buffer->max_count ); \
	(*entry) = &buffer->entries[buffer->write_pos]; \
} \
 \
void NAME##_push_end( \
		NAME##_t* buffer \
) \
{ \
	buffer->write_pos = (buffer->write_pos+1) % buffer->max_count; \
	buffer->count++; \
}

# ifndef GLOBAL_H
# define GLOBAL_H

#include <assert.h>
#include <stdbool.h>

#define STR_BUFFER_SIZE 1024

/***********************
 * Types
 ***********************/

typedef unsigned char byte_t;

typedef unsigned int uint;

typedef enum {
	RET_SUCCESS,
	RET_FAILURE
} ret_t;

#define CALLOC( PTR, COUNT, SIZE) {\
	assert( PTR == NULL ); \
	PTR = calloc( COUNT, SIZE ); \
	if( PTR == NULL ) { \
		fprintf( stderr, "calloc failed! out of memory!" ); \
		exit( EXIT_FAILURE ); \
	} \
}

#define FREE( PTR ) {\
	if( PTR != NULL ) { \
		free( PTR ); \
		PTR = NULL; \
	} \
}

#endif

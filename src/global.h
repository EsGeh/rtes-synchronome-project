# ifndef GLOBAL_H
# define GLOBAL_H

#define STR_BUFFER_SIZE 1024

/***********************
 * Types
 ***********************/

typedef enum { false, true } bool;

typedef enum {
	RET_SUCCESS,
	RET_FAILURE
} ret_t;

#define FREE( PTR ) {\
	if( PTR != NULL ) { \
		free( PTR ); \
		PTR = NULL; \
	} \
}

#endif

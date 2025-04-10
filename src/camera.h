#ifndef CAMERA_H
#define CAMERA_H

#include "global.h"
#include <stddef.h>


/********************
 * Types
********************/

typedef struct {
	void* data;
	size_t size;
} buffer_t;

typedef struct
{
	buffer_t* buffers;
	unsigned int count;
} buffer_container_t;

typedef struct {
	char dev_name[STR_BUFFER_SIZE];
	int dev_file;
	buffer_container_t buffer_container;
} camera_t;

/********************
 * Functions
********************/

ret_t camera_init(
		camera_t* camera,
		const char* dev_name
);
ret_t camera_exit(
		camera_t* camera
);

char* camera_error();
void camera_reset_error();

#endif

#ifndef CAMERA_H
#define CAMERA_H

#include "global.h"


typedef struct {
	char dev_name[STR_BUFFER_SIZE];
	int dev_file;
} camera_t;

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

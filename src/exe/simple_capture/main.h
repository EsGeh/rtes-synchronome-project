#include "lib/camera.h"

typedef struct {
	frame_size_t size;
	frame_interval_t acq_interval;
	char* dev_name;
	char* output_dir;
} capture_args_t;


ret_t capture_single(capture_args_t* args);

#include "acq_queue.h"
#include "select_queue.h"

#include <semaphore.h>

ret_t select_run(
		// sem_t* sem,
		// bool* stop,
		acq_queue_t* input_queue,
		select_queue_t* output_queue,
		dump_frame_func_t dump_frame
		/*
		byte_t* rgb_buffer,
		uint rgb_buffer_size,
		const char* output_dir
		*/
);

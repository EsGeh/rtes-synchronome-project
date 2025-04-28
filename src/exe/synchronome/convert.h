#include "acq_queue.h"
#include "select_queue.h"

#include <semaphore.h>

ret_t convert_run(
		select_queue_t* input_queue,
		dump_frame_func_t dump_frame
);

#include "exe/synchronome/acq_queue.h"
#include "exe/synchronome/select_queue.h"
#include "frame_acq.h"

#include "lib/output.h"
#include "lib/global.h"


ret_t convert_run(
		select_queue_t* input_queue,
		dump_frame_func_t dump_frame
)
{
	acq_entry_t entry;
	while( true ) {
		select_queue_wait( input_queue );
		if( select_queue_should_stop( input_queue ) ) {
			log_info( "convert: stopping\n" );
			break;
		}
		log_info( "convert: reading frame\n" );
		select_queue_peek(input_queue, &entry);
		{
			log_info( "convert: received\n" );
		}
		select_queue_pop(input_queue);
		dump_frame( entry.frame );
	}
	return RET_SUCCESS;
}

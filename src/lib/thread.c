#include "thread.h"
#include "output.h"
#include <string.h>

#include <sched.h>
#include <errno.h>

ret_t thread_create(
		const char* name,
		pthread_t* thread,
		void* (*func)(void* p),
		void* arg
)
{
	pthread_attr_t thread_attrs;
	pthread_attr_init( &thread_attrs );
	
	if( pthread_create(
		thread,					// pointer to thread descriptor
		&thread_attrs,		// scheduling details
	  func,	      			// thread function entry point
		arg 							// parameters to pass in
	) )
	{
		log_error( "pthread_create: %s", strerror( errno ) );
		return RET_FAILURE;
	}
	if( pthread_setname_np( *thread, name ) ) {
		// not a fatal error, continue:
		log_error( "pthread_setname_np: %s", strerror( errno ) );
	}
	return RET_SUCCESS;
}

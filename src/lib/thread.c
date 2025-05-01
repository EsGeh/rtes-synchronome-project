#include "thread.h"
#include "lib/global.h"
#include "output.h"
#include <string.h>

#include <sched.h>
#include <errno.h>
#include <sys/sysinfo.h>


ret_t thread_create(
		const char* name,
		pthread_t* thread,
		void* (*func)(void* p),
		void* arg,
		const int sched_policy,
		const int priority,
		const int cpu_core
)
{
	pthread_attr_t thread_attrs;
	if( 0 != pthread_attr_init( &thread_attrs ) ) {
		log_error( "pthread_attr_init: %s\n", strerror( errno ) );
		return RET_FAILURE;
	}
	int ret =  pthread_attr_setinheritsched( &thread_attrs, PTHREAD_EXPLICIT_SCHED );
	if( ret != 0 ) {
		log_error( "pthread_attr_setinheritsched: %s\n", strerror( ret ) );
		return RET_FAILURE;
	}
	if( sched_policy != SCHED_NORMAL )
	{ // set policy:
		int ret = pthread_attr_setschedpolicy( &thread_attrs, SCHED_FIFO );
		if( ret != 0 ) {
			log_error( "pthread_attr_setschedpolicy: %s\n", strerror( ret ) );
			return RET_FAILURE;
		}
	}
	// set priority:
	if( sched_policy == SCHED_NORMAL && priority == -1 ) {
	}
	else
	{
		struct sched_param sched_param;
		if( priority == -1 ) {
			sched_param.sched_priority = thread_get_min_priority(sched_policy);
		}
		else {
			sched_param.sched_priority = priority;
		}
		// sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
		int ret =  pthread_attr_setschedparam( &thread_attrs, &sched_param );
		if( ret != 0 ) {
			log_error( "pthread_attr_setschedparam: %s\n", strerror( ret ) );
			return RET_FAILURE;
		}
	}
	// set cpu affinity:
	if( cpu_core >= 0 ) {
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		CPU_SET(
				cpu_core,
				&cpu_set
		);
		int ret = pthread_attr_setaffinity_np(&thread_attrs, sizeof(cpu_set_t), &cpu_set);
		if( ret != 0 ){
			log_error( "pthread_attr_setaffinity_np: %s\n", strerror( ret ) );
			return RET_FAILURE;
		}
	}
	{
		int ret = pthread_create(
			thread,					// pointer to thread descriptor
			&thread_attrs,		// scheduling details
			func,	      			// thread function entry point
			arg 							// parameters to pass in
		);
		if( ret != 0 ) {
			log_error( "pthread_create: %s\n", strerror( ret ) );
			return RET_FAILURE;
		}
	}
	{
		int ret =  pthread_setname_np( *thread, name );
		if( ret != 0 ){
			// not a fatal error, continue:
			log_error( "pthread_setname_np: %s\n", strerror( ret ) );
		}
	}
	return RET_SUCCESS;
}

ret_t thread_join_ret(
		pthread_t thread
)
{
	ret_t* ret;
	int err = pthread_join(
			thread,
			(void**)&ret
	);
	if( err != 0 ) {
		log_error( "pthread_join: %s\n", strerror( err ) );
		return RET_FAILURE;
	}
	return *ret;
}

int thread_get_max_priority(const int sched_policy)
{
	return sched_get_priority_max(sched_policy);
}

int thread_get_min_priority(const int sched_policy)
{
	return sched_get_priority_min(sched_policy);
}

uint thread_get_cpu_count()
{
	return get_nprocs();
}

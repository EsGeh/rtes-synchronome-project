#pragma once

#include "global.h"
#include <pthread.h>

ret_t thread_create(
		const char* name,
		pthread_t* thread,
		void* (*func)(void* p),
		void* arg
);

ret_t thread_join_ret(
		pthread_t thread
);

#pragma once

#include "global.h"
#include <pthread.h>

ret_t thread_create(
		const char* name,
		pthread_t* thread,
		void* (*func)(void* p),
		void* arg,
		const int sched_policy, // SCHED_NORMAL for dont set
		const int priority, // -1 for dont set
		const int cpu // -1 for dont set
);

ret_t thread_join_ret(
		pthread_t thread
);

int thread_get_max_priority(const int sched_policy);
int thread_get_min_priority(const int sched_policy);

uint thread_get_cpu_count();

void thread_info(const char* thread_name);

#pragma once

#include <errno.h>
#include <semaphore.h>


static inline int sem_wait_nointr(sem_t* sem) {
	while( sem_wait(sem) ) {
		if( errno == EINTR ) errno=0;
		else return -1;
	}
	return 0;
}

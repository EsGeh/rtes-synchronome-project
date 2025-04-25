#include "time.h"
#include "global.h"
#include "output.h"

#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

struct timespec start_time;

// internal utilities:
void nsec_to_timespec(
		const NSEC ns,
		struct timespec* result
);


void time_init(void)
{
  clock_gettime(MEASUREMENT_CLOCK, &start_time);
}

struct timespec time_measure_current_time(void)
{
	struct timespec current_time;
	clock_gettime(MEASUREMENT_CLOCK, &current_time);
	struct timespec current_time_rel;
	time_delta(
			&current_time,
			&start_time,
			&current_time_rel
	);
	return current_time_rel;
}

ret_t time_add_timer(
		void (*timer_callback)(int),
		const USEC period_us
)
{
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa));
	sa.sa_handler = timer_callback;
	sigemptyset( &sa.sa_mask );
	if( -1 == sigaction( SIGALRM, &sa, NULL ) ) {
		log_error( "'sigaction': %s", strerror(errno) );
		return RET_FAILURE;
	}
	// repeat running scheduler
	// in regular intervals:
	{
		timer_t timer;
		timer_create(TIMER_CLOCK, NULL, &timer);
		static struct itimerspec timer_spec;
		{
			nsec_to_timespec(
					period_us * 1000,
					&timer_spec.it_interval
			);
			nsec_to_timespec(
					period_us * 1000,
					&timer_spec.it_value
			);
		}
		timer_settime(
				timer,
				0,
				&timer_spec,
				NULL
		);
	}
	return RET_SUCCESS;
}

void time_sleep(
		const USEC period_us
)
{
	struct timespec remaining_time = {
		.tv_sec = period_us / 1000 / 1000,
		.tv_nsec = period_us * 1000
	};
	int ret = 0;
	do
	{
		ret = nanosleep(
			&remaining_time,
			&remaining_time
		);
	}
	while( ret != 0 );
}

#define NSEC_PER_SEC (1000 * 1000 * 1000)

void nsec_to_timespec(
		const NSEC ns,
		struct timespec* result
)
{
	result->tv_sec = ns / 1000 / 1000 / 1000;
	result->tv_nsec = ns % (1000 * 1000 * 1000);
}

void time_delta(
		const struct timespec *stop,
		const struct timespec *start,
		struct timespec *delta_t
)
{
	int dt_sec = stop->tv_sec - start->tv_sec;
	int dt_nsec = stop->tv_nsec - start->tv_nsec;

	while( dt_nsec > NSEC_PER_SEC )
	{
		dt_sec ++;
		dt_nsec -= NSEC_PER_SEC;
	};
	while( dt_nsec < 0 )
	{
		dt_sec --;
		dt_nsec += NSEC_PER_SEC;
	};
	delta_t->tv_sec = dt_sec;
	delta_t->tv_nsec = dt_nsec;
}

USEC time_us_from_timespec(
		const struct timespec* delta
)
{
	USEC ret = 0;
	ret += (delta->tv_sec * 1000*1000);
	ret += (delta->tv_nsec / 1000);
	return ret;
}

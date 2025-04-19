#ifndef TIME_H
#define TIME_H

// #include "global.h"

#define MEASUREMENT_CLOCK CLOCK_MONOTONIC_RAW
#define TIMER_CLOCK CLOCK_REALTIME

/***********************
 * Types
 ***********************/

// represents
// MICROSECONDS ( = 10^(-6) s )
typedef long int USEC;
typedef long long int NSEC;
typedef struct timespec timeval_t;

/***********************
 * Measure elapsed time
 ***********************/

// init module:
void time_init(void);

// get elapsed time since 'time_init()':
struct timespec time_measure_current_time(void);


/***********************
 * Repeat at const frequency
 ***********************/

void time_repeat(
		void (*timer_callback)(int),
		const USEC period_us
);

void time_sleep(
		const USEC period_us
);

/***********************
 * Utilities
 ***********************/

// calculate time difference:
// 		delta_t = stop - start
void time_delta(
		const struct timespec* stop,
		const struct timespec* start,
		struct timespec* delta_t
);

USEC time_us_from_timespec(
		const struct timespec* delta
);

#endif

#ifndef TIME_H
#define TIME_H

#include "global.h"

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
timeval_t time_measure_current_time(void);

long long int time_measure_current_time_us(void);


/***********************
 * Repeat at const frequency
 ***********************/

ret_t time_add_timer(
		void (*timer_callback)(int),
		const USEC period_us
);

void time_sleep(
		const USEC period_us
);

/***********************
 * Utilities
 ***********************/

void  time_normalize_timeval(
		struct timespec* time
);

// calculate time difference:
// 		delta = stop - start
void time_delta(
		const struct timespec* stop,
		const struct timespec* start,
		struct timespec* delta
);

// calculate time sum:
//   result = t0 + t1
void time_add(
		const struct timespec* t0,
		const struct timespec* t1,
		struct timespec* result
);

// calculate time sum:
//   result = t0 + t1
void time_add_us(
		const struct timespec* t0,
		const USEC t1_us,
		struct timespec* result
);

void time_mul_i(
		const struct timespec* t0,
		const uint i,
		struct timespec* result
);

USEC time_us_from_timespec(
		const struct timespec* delta
);

void time_timespec_from_us(
		const USEC usec,
		struct timespec* time
);

#endif

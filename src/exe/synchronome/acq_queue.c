#include "acq_queue.h"

#include "lib/output.h"

#include <string.h>


#define DBG_LOG(fmt,...) log_verbose(fmt, ## __VA_ARGS__)
#define ERR_LOG(fmt,...) { \
	log_error(fmt, ## __VA_ARGS__); \
	exit(1); \
}

DEF_SPSC_QUEUE(acq_queue,acq_entry_t,DBG_LOG,ERR_LOG)

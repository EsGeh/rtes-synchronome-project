#pragma once

#include "exe/synchronome/queues/acq_queue.h"

#include <semaphore.h>


typedef acq_entry_t select_entry_t;

DECL_SPSC_QUEUE(select_queue,select_entry_t)

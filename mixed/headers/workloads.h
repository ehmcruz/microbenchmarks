#ifndef _microbenchmark_workloads_h
	#define _microbenchmark_workloads_h

#include "mixed.h"
extern thread_data_t *threads;
extern uint32_t nt;
extern volatile int alive;

void workload_harmonic (thread_data_t *t);
void workload_pointer_chasing_init_buffer (thread_data_t *t, uint32_t buffer_size);
void workload_pointer_chasing (thread_data_t *t);
void workload_vsum_init_buffer (thread_data_t *t);
void workload_vsum (thread_data_t *t);
#endif

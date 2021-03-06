#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#ifndef _microbenchmark_mixed_h
	#define _microbenchmark_mixed_h
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>


#define STEP 128
#define CACHE_LINE_SIZE 128
#define N_WORKLOADS 4

typedef enum workload_t {
    WORKLOAD_HARMONIC,
    WORKLOAD_POINTER_CHASING,
	WORKLOAD_VSUM,
	WORKLOAD_FIBONACCI_IT,
	WORKLOAD_NONE,
	WORKLOAD_IDLE,
    WORKLOAD_NTYPES
} workload_t;


struct list_el_t {
    struct list_el_t *next;
    uint64_t v;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct list_el_t list_el_t;

struct thread_data_t {
    int id;
    workload_t type;
    uint64_t nloops;
    pid_t tid;
    int cpu;

    double v;
    uint64_t v2;
	uint64_t *buffer;
    list_el_t *list;
}  __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct thread_data_t thread_data_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>

#if defined(_OPENMP)
	#include <omp.h>
	#define BUSY_WAIT
#else
	#include <pthread.h>
#endif

#ifdef DEBUG
	#undef DEBUG
#endif

#define DEBUG
#define KEEP_ALIVE

#ifdef DEBUG
	#define DPRINTF(...) printf(__VA_ARGS__)
#else
	#define DPRINTF(...)
#endif

enum {
	REMAP_PHASE = 0,
	REMAP_IT_WRITER = 1,
	REMAP_IT_READER = 2
};

static int vsize, npairs, nint, nphases, nthreads;

#ifdef KEEP_ALIVE
	static volatile int completed;
#endif

#ifndef CACHE_LINE_SIZE
	#define CACHE_LINE_SIZE 64
#endif

struct element_t {
	int el;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));;

typedef struct element_t element_t;

struct resource_t {
	volatile element_t *v;

	#ifdef BUSY_WAIT
		volatile int ready;
		volatile int hold;
	#else
		volatile int is_reading;
		volatile int is_writing;
		pthread_mutex_t mutex;
		pthread_cond_t cond_is_writing;
		pthread_cond_t cond_is_reading;
	#endif
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct resource_t resource_t;

static resource_t *r;

#ifdef BUSY_WAIT
	static char version[] = "omp-busy-wait";
#else
	static char version[] = "pthreads-sleep";

	static pthread_mutex_t phase_mutex;
	static pthread_cond_t phase_cond;
#endif

#ifdef KEEP_ALIVE
	#define OTHERCOND || completed < npairs
#else
	#define OTHERCOND
#endif

#define CPU_PAUSE __asm__ __volatile__ ("pause");

static void reader_wait(resource_t *p)
{
	#ifdef BUSY_WAIT
		while (p->hold != 1) {
			CPU_PAUSE
		}
	#else
		pthread_mutex_lock(&p->mutex);
		if (p->is_writing) {
			pthread_cond_wait(&p->cond_is_writing, &p->mutex);
		}
		p->is_reading = 1;
		pthread_mutex_unlock(&p->mutex);
	#endif
}

static void reader_finished(resource_t *p)
{
	#ifdef BUSY_WAIT
		p->hold = 0;
	#else
	#endif
}

static void writer_wait(resource_t *p)
{
	#ifdef BUSY_WAIT
		while (p->hold != 0) {
			CPU_PAUSE
		}
	#else
	#endif
}

static void writer_finished(resource_t *p)
{
	#ifdef BUSY_WAIT
		p->hold = 1;
	#else
		pthread_mutex_lock(&p->mutex);
		p->is_writing = 0;
		pthread_cond_signal(&p->cond_is_writing);
		pthread_mutex_unlock(&p->mutex);
	#endif
}

static void reader(resource_t *p, int id, int phase)
{
	int i, j, z = 0;

	#ifdef BUSY_WAIT
		#pragma omp atomic
		p->ready++;

		while (p->ready < 2);
	#endif

	for (i=0; i<nint OTHERCOND; i++) {
		reader_wait(p);

		for (j=0; j<vsize; j++) {
			z += p->v[j].el;
		}
		reader_finished(p);
		#ifdef KEEP_ALIVE
			if (i == (nint - 1)) {
				#pragma omp atomic
				completed++;
			}
		#endif
	}
}

static void writer(resource_t *p, int id, int phase)
{
	int i, j;

	#ifdef BUSY_WAIT
		#pragma omp atomic
		p->ready++;

		while (p->ready < 2);
	#endif

	for (i=0; i<nint OTHERCOND; i++) {
		writer_wait(p);

		for (j=0; j<vsize; j++) {
			p->v[j].el = i + j;
		}
		writer_finished(p);
	}
}

static void run_phase(int i, int id)
{
	resource_t *p;

	if ((i % 2) == 0) {
		/*
			0,1 -> 0
			2,3 -> 1

			0,1 -> 0
			2,3 -> 1
			4,5 -> 2
			6,7 -> 3
			...
		*/
		p = r + (id / 2);

		if ((id % 2) == 0)
			reader(p, id, i);
		else
			writer(p, id, i);
	}
	else {
		/*
			0,2 -> 0
			1,3 -> 1

			0,4 -> 0
			1,5 -> 1
			2,6 -> 2
			3,7 -> 3
			...
		*/
		p = r + (id % npairs);

		if ((id / npairs) == 0)
			reader(p, id, i);
		else
			writer(p, id, i);
	}
}

static void prepare_phase(int i)
{
	int j;

	for (j=0; j<npairs; j++) {
		#ifdef BUSY_WAIT
			r[j].ready = 0;
			r[j].hold = 0;
		#else
			r[j].is_writing = 1;
			r[j].is_reading = 0;
		#endif
	}

	DPRINTF("phase %i\n", i);

	#ifdef KEEP_ALIVE
		completed = 0;
	#endif
}

#ifndef BUSY_WAIT
static void* pthreads_callback(void *data)
{
	int id = (int)data;
	int i;
	static volatile int nready = 0;
	
	for (i=0; i<nphases; i++) {
		pthread_mutex_lock(&phase_mutex);
		nready++;
		if (nready < nthreads) {
			pthread_cond_wait(&phase_cond, &phase_mutex);
		}
		else {
			int r;

			nready = 0; // for next phase
			prepare_phase(i);
			r = pthread_cond_broadcast(&phase_cond);
			assert(!r);
		}
		pthread_mutex_unlock(&phase_mutex);

		run_phase(i, id);
	}

	return NULL;
}
#endif

int main(int argc, char **argv)
{
	int i;
	struct timeval timer_begin_app, timer_end_app;
	double elapsed;
	#ifndef BUSY_WAIT
		pthread_t *threads;
	#endif

	if (argc != 5) {
		printf("%s vsize nint nphases npairs\n", argv[0]);
		exit(1);
	}

	assert(sizeof(element_t) == CACHE_LINE_SIZE);

	vsize = atoi(argv[1]);
	nint = atoi(argv[2]);
	nphases = atoi(argv[3]);
	npairs = atoi(argv[4]);

	nthreads = npairs * 2;

	#ifdef BUSY_WAIT
		omp_set_num_threads(nthreads);
	#else
		pthread_mutex_init(&phase_mutex, NULL);
		pthread_cond_init (&phase_cond, NULL);
		threads = (pthread_t*)calloc(nthreads, sizeof(pthread_t));
		assert(threads != NULL);
	#endif

	r = calloc(npairs, sizeof(resource_t));
	assert(r != NULL);
	
	for (i=0; i<npairs; i++) {
		r[i].v = (element_t*)calloc(vsize, sizeof(element_t));

		assert(r[i].v != NULL);
		#ifdef BUSY_WAIT
			r[i].ready = 0;
			r[i].hold = 0;
		#else
			pthread_mutex_init(&r[i].mutex, NULL);
			pthread_cond_init (&r[i].cond_is_writing, NULL);
			pthread_cond_init (&r[i].cond_is_reading, NULL);
		#endif
	}

	DPRINTF("%s %lu kbytes vector (%i elements), %i iterations, %i phases, %i threads\n", version, (vsize * sizeof(element_t)) / 1024, vsize, nint, nphases, nthreads);
	
	gettimeofday(&timer_begin_app, NULL);

#ifdef BUSY_WAIT
	for (i=0; i<nphases; i++) {
		prepare_phase(i);

		#pragma omp parallel
		{
			run_phase(i, omp_get_thread_num());
		}
	}
#else
	for (i=1; i<nthreads; i++) {
		pthread_create(&threads[i], NULL, pthreads_callback, (void *)i);
	}
	pthreads_callback(0);
	
	for (i=1; i<nthreads; i++) {
		pthread_join(threads[i], NULL);
	}
#endif

	gettimeofday(&timer_end_app, NULL);
	
	elapsed = timer_end_app.tv_sec - timer_begin_app.tv_sec + (timer_end_app.tv_usec - timer_begin_app.tv_usec) / 1000000.0;
	
	printf("Execution time: %.3f seconds\n", elapsed);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if defined(_OPENMP)
	#include <omp.h>
	#define BUSY_WAIT
#else
	#include <pthread.h>
#endif

/*#if defined(PERFECT_REMAP) || defined(WITH_DATAMAPPING)*/
	#include <libmapping-interface.h>
/*	#undef PERFECT_REMAP*/
/*#endif*/

#if defined(WITH_DATAMAPPING)
	#include <numa.h>
#endif

#ifdef DEBUG
	#undef DEBUG
#endif

#define DEBUG
#define KEEP_ALIVE

//#define REALLOC_DATA

#ifdef DEBUG
	#define DPRINTF(...) printf(__VA_ARGS__)
#else
	#define DPRINTF(...)
#endif

#define CFG_PAGE_SIZE (16 * 1024)

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

typedef struct element_t {
	int el;
	char fill_line[ CACHE_LINE_SIZE - sizeof(int) ];
} element_t;

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

	volatile char empty[CACHE_LINE_SIZE*2]; // force different cache lines to avoid false sharing
} __attribute__ ((aligned (CFG_PAGE_SIZE)));

typedef struct resource_t resource_t;

/*#if defined(PERFECT_REMAP)*/
	static uint32_t *libmapping_thread_pos;
/*#endif*/

static resource_t *r;

#if defined(WITH_DATAMAPPING)
	static uint32_t *nodes_of_resources;
#endif

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

static void reader_wait(resource_t *p)
{
	#ifdef BUSY_WAIT
		while (p->hold != 1);
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
		while (p->hold != 0);
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
		#ifndef PERFECT_REMAP
			#ifdef LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
				libmapping_remap(REMAP_IT_READER, ((i + nint*phase) << 8) | id);
			#elif defined(LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_REALMACHINESIDE)
				if (id == 0) {
					thread_mapping_t *tm;
					uint32_t step;
					int code;

					step = ((i + nint*phase) << 8) | id;

					tm = get_comm_pattern(REMAP_IT_READER, step);

					if (tm == NULL) {
						if (i < nint) {
							assert(0);
						}
					}
					else {
						code = libmapping_remap_check_migrate(tm);

						#ifdef DEBUG
							if (code == LIBMAPPING_REMAP_MIGRATED)
								DPRINTF("\tstep %i MIGRATED\n", i);
							//else
							//	DPRINTF("\tstep %i\n", i);
						#endif
					}
				}
			#elif defined(LIBMAPPING_REAL_REMAP_SIMICS)
				if (id == 0) {
					int code;
					code = libmapping_remap_check_migrate();
					#ifdef DEBUG
						if (code == LIBMAPPING_REMAP_MIGRATED)
							DPRINTF("\tstep %i MIGRATED\n", i);
						//else
						//	DPRINTF("\tstep %i\n", i);
					#endif
				}
			#endif
		#endif
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
		#ifdef LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
			libmapping_remap(REMAP_IT_WRITER, ((i + nint*phase) << 8) | id);
		#endif
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

#ifdef PERFECT_REMAP
static void perfect_remap(int i)
{
	int j;

	if ((i % 2) == 0) {
		for (j=0; j<nthreads; j++) {
			libmapping_set_aff_thread_by_order_id(libmapping_thread_pos[j], libmapping_topology_get_best_pu_by_pos(j));
		}
	}
	else {
		int tid;
		for (j=0; j<nthreads; j++) {
			if ((j % 2) == 0)
				tid = j / 2;
			else
				tid = (nthreads / 2) + (j / 2);
			libmapping_set_aff_thread_by_order_id(libmapping_thread_pos[tid], libmapping_topology_get_best_pu_by_pos(j));
		}
		/*
		libmapping_set_aff_of_thread(0, 0); j = 0
		libmapping_set_aff_of_thread(4, 4); j = 1

		libmapping_set_aff_of_thread(1, 2); j = 2
		libmapping_set_aff_of_thread(5, 6); j = 3

		libmapping_set_aff_of_thread(2, 1); j = 4
		libmapping_set_aff_of_thread(6, 5); j = 5

		libmapping_set_aff_of_thread(3, 3); j = 6
		libmapping_set_aff_of_thread(7, 7); j = 7
		*/
	}

	#if defined(DEBUG)
		DPRINTF("perfect remap to \n");
		for (j=0; j<nthreads; j++) {
			DPRINTF("%i,", libmapping_get_aff_thread_by_order_id(libmapping_thread_pos[j]));
		}
		DPRINTF("\n");
	#endif
}
#endif

static void prepare_phase(int i)
{
	int j;

	#ifdef LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
		libmapping_remap(REMAP_PHASE, i);
	#endif

#ifdef REALLOC_DATA
	for (j=0; j<npairs; j++) {
		r[j].v = (element_t*)calloc(vsize, sizeof(element_t));
		assert(r[j].v != NULL);
	}
#endif

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

	#ifdef PERFECT_REMAP
		perfect_remap(i);
	#endif

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
	
	libmapping_thread_pos[id] = libmapping_get_current_thread_order_id();

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
	
	printf("libmapping_topology_get_number_of_pus()=%i\n", libmapping_topology_get_number_of_pus());

/*#if defined(PERFECT_REMAP)*/
	assert(nthreads <= libmapping_topology_get_number_of_pus());
	libmapping_thread_pos = (uint32_t*)calloc(nthreads, sizeof(uint32_t));
	assert(libmapping_thread_pos != NULL);
/*#endif*/

#if defined(WITH_DATAMAPPING)
	printf("there are %u NUMA nodes\n", libmapping_topology_get_number_of_numa_nodes());
	
	nodes_of_resources = (uint32_t*)calloc(npairs, sizeof(uint32_t));
	assert(nodes_of_resources != NULL);
	
	if (libmapping_env_to_vector("PCNODES", (int32_t*)nodes_of_resources, npairs, 0) != npairs) {
		printf("env PCNODES must be a list of NUMA nodes, one element per pair of threads\n");
		exit(1);
	}
	
	for (i=0; i<npairs; i++) {
		assert(nodes_of_resources[i] < libmapping_topology_get_number_of_numa_nodes());
	}
#endif

	for (i=0; i<npairs; i++) {
	#if defined(WITH_DATAMAPPING)
		r[i].v = (element_t*)numa_alloc_onnode(vsize * sizeof(element_t), nodes_of_resources[i]);
	#else
		r[i].v = (element_t*)calloc(vsize, sizeof(element_t));
	#endif
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

	#if defined(PERFECT_REMAP)
		libmapping_set_dynamic_mode(0);
		
		#ifdef BUSY_WAIT
			#pragma omp parallel
			{
				libmapping_thread_pos[ omp_get_thread_num() ] = libmapping_get_current_thread_order_id();
			}
		#endif
	#endif

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

	return 0;
}

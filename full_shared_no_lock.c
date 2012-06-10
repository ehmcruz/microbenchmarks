#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <libmapping.h>

#if defined(LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_REALMACHINESIDE) || defined(LIBMAPPING_REAL_REMAP_SIMICS)
	#include <libremap.h>
#elif defined(PERFECT_REMAP)
	#include <map_algorithm.h>
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

typedef struct element_t {
	int el;
	char fill_line[ CACHE_LINE_SIZE - sizeof(int) ];
} element_t;

typedef struct resource_t {
	volatile element_t *v;
	volatile int ready;
	volatile int hold;
	volatile char empty[CACHE_LINE_SIZE*2]; // force different cache lines to avoid false sharing
} resource_t;

resource_t *r;

#ifdef KEEP_ALIVE
	#define OTHERCOND || completed < npairs
#else
	#define OTHERCOND
#endif

void reader(resource_t *p, int id, int phase)
{
	int i, j, z = 0;
	
	#pragma omp atomic
	p->ready++;
	
	while (p->ready < 2);
	
	for (i=0; i<nint OTHERCOND; i++) {
		while (p->hold != 1);
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

					if (i < nint && tm == NULL) {
						assert(0);
					}

					code = libmapping_remap_check_migrate(tm);
								
					#ifdef DEBUG
						if (code == LIBMAPPING_REMAP_MIGRATED)
							DPRINTF("\tstep %i MIGRATED\n", i);
						//else
						//	DPRINTF("\tstep %i\n", i);
					#endif
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
		p->hold = 0;
		#ifdef KEEP_ALIVE
			if (i == (nint - 1)) {
				#pragma omp atomic
				completed++;
			}
		#endif
	}
}

void writer(resource_t *p, int id, int phase)
{
	int i, j;

	#pragma omp atomic
	p->ready++;
	
	while (p->ready < 2);

	for (i=0; i<nint OTHERCOND; i++) {
		while (p->hold != 0);
		#ifdef LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
			libmapping_remap(REMAP_IT_WRITER, ((i + nint*phase) << 8) | id);
		#endif
		for (j=0; j<vsize; j++) {
			p->v[j].el = i + j;
		}
		p->hold = 1;
	}
}

int main(int argc, char **argv)
{
	int i, j;
		
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
	omp_set_num_threads(nthreads);
	
	r = calloc(npairs, sizeof(resource_t));
	assert(r != NULL);

	for (i=0; i<npairs; i++) {
		r[i].ready = 0;
		r[i].v = (element_t*)calloc(vsize, sizeof(element_t));
		r[i].hold = 0;
		assert(r[i].v != NULL);
	}
	
	DPRINTF("%i kbytes vector (%i elements), %i iterations, %i phases, %i threads\n", (vsize * sizeof(element_t)) / 1024, vsize, nint, nphases, nthreads);
	
	#if defined(PERFECT_REMAP) && (defined(LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_REALMACHINESIDE) || defined(LIBMAPPING_REAL_REMAP_SIMICS))
		libmapping_set_allow_dynamic(0);
	#endif

	libmapping_omp_automate();
	#ifdef PERFECT_REMAP
		wrapper_load_hierarchy_from_env();
	#endif
		
	for (i=0; i<nphases; i++) {
		#ifdef LIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
			libmapping_remap(REMAP_PHASE, i);
		#endif

		for (j=0; j<npairs; j++) {
			r[j].ready = 0;
			r[j].hold = 0;
		}

		DPRINTF("phase %i\n", i);		

		#ifdef PERFECT_REMAP			
			if ((i % 2) == 0) {
				for (j=0; j<nthreads; j++) {
					libmapping_set_aff_of_thread(j, wrapper_get_coreid_from_hierarchy(j));
				}
			}
			else {
				int tid;
				for (j=0; j<nthreads; j++) {
					if ((j % 2) == 0)
						tid = j / 2;
					else
						tid = (nthreads / 2) + (j / 2);
					libmapping_set_aff_of_thread(tid, wrapper_get_coreid_from_hierarchy(j));
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

			#ifdef DEBUG
				DPRINTF("perfect remap to \n");
				for (j=0; j<nthreads; j++) {
					DPRINTF("%i,", libmapping_get_aff_of_thread(j));
				}
				DPRINTF("\n");
			#endif
		#endif
		
		#ifdef KEEP_ALIVE
			completed = 0;
		#endif

		#pragma omp parallel
		{
			int id;
			resource_t *p;
		
			id = omp_get_thread_num();
			
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
	}

	libmapping_omp_automate_finish();
		
	return 0;
}

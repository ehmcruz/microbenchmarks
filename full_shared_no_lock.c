#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "libmapping.h"

int vsize, npairs, nint, nphases;

typedef struct resource_t {
	volatile int *v;
	volatile int ready;
	volatile int hold;
	volatile char empty[128]; // force different cache lines to avoid false sharing
} resource_t;

resource_t *r;

void reader(resource_t *p)
{
	int i, j, z = 0;
	
	#pragma omp atomic
	p->ready++;
	
	while (p->ready < 2);
	
	for (i=0; i<nint; i++) {
		while (p->hold != 1);
		for (j=0; j<vsize; j++) {
			z += p->v[j];
		}
		p->hold = 0;
	}
}

void writer(resource_t *p)
{
	int i, j;

	#pragma omp atomic
	p->ready++;
	
	while (p->ready < 2);

	for (i=0; i<nint; i++) {
		while (p->hold != 0);
		for (j=0; j<vsize; j++) {
			p->v[j] = i + j;
		}
		p->hold = 1;
	}
}

int main(int argc, char **argv)
{
	int i;
		
	if (argc != 5) {
		printf("%s vsize nint nphases npairs\n", argv[0]);
		exit(1);
	}
	
	vsize = atoi(argv[1]);
	nint = atoi(argv[2]);
	nphases = atoi(argv[3]);
	npairs = atoi(argv[4]);
	
	omp_set_num_threads(npairs*2);
	
	r = calloc(npairs, sizeof(resource_t));
	assert(r != NULL);

	for (i=0; i<npairs; i++) {
		r[i].ready = 0;
		r[i].v = calloc(vsize, sizeof(int));
		r[i].hold = 0;
		assert(r[i].v != NULL);
	}

	libmapping_omp_automate();
	
	for (i=0; i<nphases; i++) {
		#ifdef REMAP
			assert(npairs == 4);
			if ((i % 2) == 0) {
				libmapping_set_aff_of_thread(0, 0);
				libmapping_set_aff_of_thread(1, 4);
				libmapping_set_aff_of_thread(2, 2);
				libmapping_set_aff_of_thread(3, 6);
				libmapping_set_aff_of_thread(4, 1);
				libmapping_set_aff_of_thread(5, 5);
				libmapping_set_aff_of_thread(6, 3);
				libmapping_set_aff_of_thread(7, 7);
			}
			else {
				libmapping_set_aff_of_thread(0, 0);
				libmapping_set_aff_of_thread(1, 2);
				libmapping_set_aff_of_thread(2, 1);
				libmapping_set_aff_of_thread(3, 3);
				libmapping_set_aff_of_thread(4, 4);
				libmapping_set_aff_of_thread(5, 6);
				libmapping_set_aff_of_thread(6, 5);
				libmapping_set_aff_of_thread(7, 7);
			}
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
					reader(p);
				else
					writer(p);
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
					reader(p);
				else
					writer(p);
			}
		}
	}

	libmapping_omp_automate_finish();
		
	return 0;
}

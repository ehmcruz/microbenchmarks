#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mapping-lib.h"

int vsize, npairs, nint;

typedef struct resource_t {
	volatile int *v;
	volatile int ready;
	volatile int hold;
} resource_t;

resource_t *r;

void reader(int id)
{
	int i, j, z = 0;
	resource_t *p = r + id/2;
	
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

void writer(int id)
{
	int i, j;
	resource_t *p = r + id/2;

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
	struct timeval timer_start, timer_end;
	double timer_spent;
		
	if (argc < 4) {
		printf("%s vsize nint npairs [affinities]\n", argv[0]);
		exit(1);
	}
	
	vsize = atoi(argv[1]);
	nint = atoi(argv[2]);
	npairs = atoi(argv[3]);
	
	c_set_number_of_threads(npairs*2);
	for (i=0; i<npairs*2; i++) {
		c_set_affinity_of_thread(i, atoi(argv[4+i]));
	}

	c_process_affinity();
	
	r = malloc(sizeof(resource_t) * npairs);
	assert(r != NULL);
	
	for (i=0; i<npairs; i++) {
		r[i].ready = 0;
		r[i].v = malloc(sizeof(int) * vsize);
		r[i].hold = 0;
		assert(r[i].v != NULL);
	}

	#pragma omp parallel
	{
		int id;
		id = omp_get_thread_num();
		simics_magic(id+1);
	}
	
	gettimeofday(&timer_start, NULL);
		
	#pragma omp parallel
	{
		int id;
		id = omp_get_thread_num();
		if (id & 0x01)
			reader(id);
		else
			writer(id);
	}

	gettimeofday(&timer_end, NULL);
	
	timer_spent = timer_end.tv_sec - timer_start.tv_sec + (timer_end.tv_usec - timer_start.tv_usec) / 1000000.0;

	printf("Time spent: %.6fs\n", timer_spent);
		
	return 0;
}

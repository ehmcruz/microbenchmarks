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
} resource_t;

resource_t *r;

void reader(resource_t *p)
{
	int i, j, z = 0;
	//resource_t *p = r + id/2;
	
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
	//resource_t *p = r + id/2;

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
		
	if (argc != 5) {
		printf("%s vsize nint nphases npairs\n", argv[0]);
		exit(1);
	}
	
	vsize = atoi(argv[1]);
	nint = atoi(argv[2]);
	nphases = atoi(argv[3]);
	npairs = atoi(argv[4]);
	
	omp_set_num_threads(npairs*2);
	
	libmapping_omp_automate();
	
	r = calloc(npairs, sizeof(resource_t));
	assert(r != NULL);
	
	for (i=0; i<npairs; i++) {
		r[i].ready = 0;
		r[i].v = calloc(vsize, sizeof(int));
		r[i].hold = 0;
		assert(r[i].v != NULL);
	}

	gettimeofday(&timer_start, NULL);
		
	#pragma omp parallel
	{
		int id;
		resource_t *p;
		
		id = omp_get_thread_num();
		p = r + id/2;
	
		if (id & 0x01)
			reader(p);
		else
			writer(p);
	}

	gettimeofday(&timer_end, NULL);
	
	timer_spent = timer_end.tv_sec - timer_start.tv_sec + (timer_end.tv_usec - timer_start.tv_usec) / 1000000.0;

	printf("Time spent: %.6fs\n", timer_spent);
	
	libmapping_omp_automate_finish();
		
	return 0;
}

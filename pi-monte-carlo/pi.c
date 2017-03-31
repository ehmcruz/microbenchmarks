#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include <sys/time.h>

#define BASE 100000000

typedef struct thread_data_t
{
	uint64_t nit;
	uint64_t count;
} thread_data_t;

uint64_t pi_monte_carlo (uint64_t nit, uint32_t tid)
{
	uint64_t i, count;
	double x, y;
	unsigned short rbuffer[3] = { tid, tid+1, tid+2 };

	count = 0;
	
	for (i=0; i<nit; i++) {
		x = erand48(rbuffer)*2.0 - 1.0; // creates floats between
		y = erand48(rbuffer)*2.0 - 1.0; // 1 and -1

		if ((x*x + y*y) < 1.0) // stone hit the pond
			count++;
	}

	return count;
}

int main (int argc, char **argv)
{
	double pi, elapsed;
	uint64_t total_it, total_count;
	thread_data_t *t;
	uint32_t nt, id, i, types;
	struct timeval timer_begin_app, timer_end_app;
	uint64_t *its;
	
	if (argc > 1) {
		types = argc - 1;
		
		its = (uint64_t*)malloc((types));
		assert(its != NULL);
	
		for (i=1; i<argc; i++)
			its[i-1] = strtoull(argv[i], NULL, 10);
	}
	else {
		static uint64_t default_it[2] = { BASE, BASE/2 };
		
		types = 2;
		its = default_it;
	}
	
	for (i=0; i<types; i++)
		printf("type %i = %llu\n", i, its[i]);
	
	#pragma omp parallel
	{
		#pragma omp single
		{
			nt = omp_get_num_threads();
			printf("there are %u threads\n", nt);
			t = malloc(nt*sizeof(thread_data_t));
			assert(t != NULL);
			gettimeofday(&timer_begin_app, NULL);
		}
		
		id = omp_get_thread_num();
		t[id].nit = its[id % types];
		t[id].count = pi_monte_carlo(t[id].nit, id);
	}
	
	gettimeofday(&timer_end_app, NULL);
	
	elapsed = timer_end_app.tv_sec - timer_begin_app.tv_sec + (timer_end_app.tv_usec - timer_begin_app.tv_usec) / 1000000.0;
	
	total_count = 0;
	total_it = 0;
	
	for (i=0; i<nt; i++) {
		total_count += t[i].count;
		total_it += t[i].nit;
	}
	
	pi = 4.0 * (double)total_count/(double)total_it;
	
	printf("pi: %f\nExecution time: %.3f seconds\n", pi, elapsed);
	
	return 0;
}

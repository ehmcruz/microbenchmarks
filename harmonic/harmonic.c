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
	uint64_t ini;
	uint64_t end;
	double v;
} thread_data_t;

double calc_harmonic (uint64_t ini, uint64_t end)
{
	uint64_t i;
	double v;

	v = 0.0;
	
	for (i=ini; i<=end; i++)
		v += 1.0 / (double)i;

	return v;
}

static void calc_imbalance (uint32_t nt, thread_data_t *t)
{
	uint64_t sum, max, min;
	uint32_t i;
	double avg;
	
	sum = t[0].nit;
	max = t[0].nit;
	min = t[0].nit;

	for (i=1; i<nt; i++) {
		sum += t[i].nit;
		
		if (t[i].nit > max)
			max = t[i].nit;
		if (t[i].nit < min)
			min = t[i].nit;
	}

	avg = (double)sum / (double)nt;
	
	printf("imbalance-avg: %.3f%%\n", 100.0 * ((double)max - avg)/(double)max);
	printf("imbalance-min: %.3f%%\n", 100.0 * ((double)max - (double)min)/(double)max);
}

int main (int argc, char **argv)
{
	double harmonic, elapsed;
	thread_data_t *t;
	uint32_t nt, id, i, types;
	struct timeval timer_begin_app, timer_end_app;
	uint64_t *its;
	
	if (argc > 1) {
		types = argc - 1;
		
		its = (uint64_t*)malloc(types*sizeof(uint64_t));
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
	
	#pragma omp parallel private(id)
	{
		#pragma omp single
		{
			uint32_t i;
			uint64_t ac;
			
			nt = omp_get_num_threads();
			printf("there are %u threads\n", nt);
			t = malloc(nt*sizeof(thread_data_t));
			assert(t != NULL);
			
			ac = 1;
			
			for (i=0; i<nt; i++) {
				t[i].ini = ac;
				t[i].nit = its[i % types];
				t[i].end = t[i].ini + t[i].nit - 1;
				ac += t[i].nit;
			}
			
			gettimeofday(&timer_begin_app, NULL);
		}
		
		id = omp_get_thread_num();
		t[id].v = calc_harmonic(t[id].ini, t[id].end);
	}
	
	gettimeofday(&timer_end_app, NULL);
	
	elapsed = timer_end_app.tv_sec - timer_begin_app.tv_sec + (timer_end_app.tv_usec - timer_begin_app.tv_usec) / 1000000.0;
	
	harmonic = 0.0;
	for (i=0; i<nt; i++)
		harmonic += t[i].v;
	
	calc_imbalance(nt, t);
	printf("harmonic: %f\nExecution time: %.3f seconds\n", harmonic, elapsed);
	
	return 0;
}

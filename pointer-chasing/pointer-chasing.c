#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include <sys/time.h>

#define CACHE_LINE_SIZE 64

#define BASE 100000000

struct list_el_t {
	struct list_el_t *next;
	uint64_t v;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct list_el_t list_el_t;

typedef struct thread_data_t {
	uint64_t nit;
	uint64_t count;
	list_el_t *list;
} thread_data_t;

static void init_buffer (thread_data_t *t, uint32_t nt, uint32_t buffer_size)
{
	uint32_t i, j, nels;
	list_el_t *el;
	
	nels = buffer_size / sizeof(list_el_t);
	
	if (!nels)
		nels = 1;
	
	for (i=0; i<nt; i++) {
		t[i].list = NULL;
		assert(posix_memalign(&t[i].list, CACHE_LINE_SIZE, nels*sizeof(list_el_t)) == 0);
		assert(t[i].list != NULL);
		
		// create circular list
		
		for (j=0; j<nels-1; j++)
			t[i].list[j].next = t[i].list + j + 1;
		t[i].list[nels-1].next = t[i].list;
		
		// initialize the list
		
		el = t[i].list;
		do {
			el->v = i;
			el = el->next;
		} while (el != t[i].list);
	}
}

static uint64_t list_walk (uint64_t nit, list_el_t *el)
{
	uint64_t i, count;

	count = 0;
	
	for (i=0; i<nit; i++) {
		count += el->v;
		el = el->next;
	}

	return count;
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
	static uint64_t default_it[2] = { BASE, BASE/2 };
	static const char *answer[] = { "bad", "ok" };
	
	double elapsed;
	thread_data_t *t;
	uint32_t nt, id, i, types, bsize, ok;
	struct timeval timer_begin_app, timer_end_app;
	uint64_t *its;
	
	if (argc == 2) {
		bsize = atoi(argv[1]) * 1024;
		types = 2;
		its = default_it;
	}
	else if (argc >= 3) {
		bsize = atoi(argv[1]) * 1024;
		
		types = argc - 2;
		
		its = (uint64_t*)malloc(types*sizeof(uint64_t));
		assert(its != NULL);
	
		for (i=2; i<argc; i++)
			its[i-2] = strtoull(argv[i], NULL, 10);
	}
	else {
		bsize = CACHE_LINE_SIZE * 16;
		types = 2;
		its = default_it;
	}
	
	printf("buffer size: %u KB\n", bsize/1024);
	for (i=0; i<types; i++)
		printf("type %i = %llu\n", i, its[i]);
	
	#pragma omp parallel private(id)
	{
		#pragma omp single
		{
			nt = omp_get_num_threads();
			printf("there are %u threads\n", nt);
			t = malloc(nt*sizeof(thread_data_t));
			assert(t != NULL);
			init_buffer(t, nt, bsize);
			gettimeofday(&timer_begin_app, NULL);
		}
		
		id = omp_get_thread_num();
		t[id].nit = its[id % types];
		t[id].count = list_walk(t[id].nit, t[id].list);
	}
	
	gettimeofday(&timer_end_app, NULL);
	
	elapsed = timer_end_app.tv_sec - timer_begin_app.tv_sec + (timer_end_app.tv_usec - timer_begin_app.tv_usec) / 1000000.0;
	
	ok = 1;
	for (i=0; i<nt && ok; i++)
		ok = (t[i].count == (t[i].nit * (uint64_t)i));
	
	calc_imbalance(nt, t);
	printf("answer is %s\nExecution time: %.3f seconds\n", answer[ok], elapsed);
	
	return 0;
}

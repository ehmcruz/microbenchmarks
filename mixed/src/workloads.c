#include "../headers/workloads.h"

#if 1
void workload_harmonic (thread_data_t *t) 
{
    uint64_t i; 
    uint32_t j; 
    double v, div; 
 
    v = 0.0; 
    i = 0;
    div = 1.0;
    
    for (i=0; alive; i+=STEP) { 
        for (j=0; j<STEP; j++) {
            v += 1.0 / div; 
            div += 1.0;
        } 
    } 
 
    t-> v = v; 
    t->nloops = i; 
}
#else
#include <immintrin.h>
#define DO_PACKED
void workload_harmonic (thread_data_t *t) 
{
#ifdef DO_PACKED
	uint64_t i; 
	uint32_t j; 
	
	double partial[4];
	__m256d ac, div, tmp, one, four;

	ac = _mm256_set_pd(0.0, 0.0, 0.0, 0.0);
	one = _mm256_set_pd(1.0, 1.0, 1.0, 1.0);
	div = _mm256_set_pd(1.0, 2.0, 3.0, 4.0);
	four = _mm256_set_pd(4.0, 4.0, 4.0, 4.0);

	for (i=0; alive; i+=STEP) { 
		for (j=0; j<STEP; j+=4) {
			tmp = _mm256_div_pd(one, div);
			ac = _mm256_add_pd(ac, tmp);
			div = _mm256_add_pd(div, four);
		}
	}
	
	_mm256_storeu_pd(partial, ac);
	t->v = partial[0] + partial[1] + partial[2] + partial[3]; 

	t->nloops = i;
#else
	uint64_t i; 
	uint32_t j; 
	
	__m128d ac, div, tmp, one;

	ac = _mm_set_sd(0.0);
	one = _mm_set_sd(1.0);
	div = _mm_set_sd(1.0);

	for (i=0; alive; i+=STEP) { 
		for (j=0; j<STEP; j+=1) {
			tmp = _mm_div_sd(one, div);
			ac = _mm_add_sd(ac, tmp);
			div = _mm_add_sd(div, one);
		}
	}
	
	_mm_store_sd(&t->v, ac);
	t->nloops = i;
#endif
}
#endif

void workload_pointer_chasing_init_buffer (thread_data_t *t, uint32_t buffer_size)
{
    uint32_t j, nels;
    list_el_t *el;
    
    nels = buffer_size / sizeof(list_el_t);
    
    if (!nels)
        nels = 1;
    
    t->list = NULL;
    assert(posix_memalign(&t->list, CACHE_LINE_SIZE, nels*sizeof(list_el_t)) == 0);
    assert(t->list != NULL);
    
    // create circular list
    
    for (j=0; j<nels-1; j++)
        t->list[j].next = t->list + j + 1;
    t->list[nels-1].next = t->list;
    
    // initialize the list
    int i=1; //só pra compilar  
    el = t->list;
    do {
        el->v = i;
        el = el->next;
        i++; //só pra compilar
    } while (el != t->list);
}

void workload_pointer_chasing (thread_data_t *t)
{
    uint64_t i, count;
    uint32_t j;

    list_el_t *el = t->list;
    count = 0;

    for (i=0; alive; i+=STEP) {
        for (j=0; j<STEP; j++) {
            count += el->v;
            el = el->next;
        }
    }

    t->v2 = count;
    t->nloops = i;
}

void workload_vsum_init_buffer (thread_data_t *t)
{
    uint32_t i, nels;

	t->buffer = NULL;
	assert(posix_memalign(&t->buffer, CACHE_LINE_SIZE, STEP*sizeof(uint64_t)) == 0);
	assert(t->buffer != NULL);


	// initialize the buffer

	for (i=0; i<STEP; i++)
		t->buffer[i] = i;
}

void workload_vsum (thread_data_t *t)
{
    uint64_t i, count;
    uint32_t j;

    count = 0;
	for (i=0; alive; i+=STEP) {
    	for (j=0; j<STEP; j++) {
			count += t->buffer[j];
    	}
	}
	
	t->nloops = i;
	t->v2 = count;
}

void workload_fibonacci_it (thread_data_t *t) 
{
	uint64_t prev, fib, tmp, i;
	uint32_t j;

	prev = 0;
	fib = 1;

	for (i=0; alive; i+=STEP) {
		for (j=0; j<STEP; j++) {
			tmp = fib;
			fib += prev;
			prev = tmp;
		}
	}
	
	t->nloops = i;
	t->v2 = fib;
}

void workload_idle (thread_data_t *t) 
{
	uint64_t i;

	for (i=0; alive; i++) {
		__asm__ __volatile__ ("pause");
		__asm__ __volatile__ ("pause");
		__asm__ __volatile__ ("pause");
		__asm__ __volatile__ ("pause");
		__asm__ __volatile__ ("pause");
	}
	
	t->nloops = i;
}


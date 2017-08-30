#include "../headers/workloads.h"
void workload_harmonic (thread_data_t *t) 
{ 
    uint64_t i; 
    uint32_t j; 
    double v; 
 
    v = 0.0; 
    i = 0; 
     
    while (alive) { 
        for (j=0; j<STEP; j++) { 
            v += 1.0 / (double)i; 
            i++; 
        } 
    } 
 
    t-> v = v; 
    t->nloops = i; 
} 

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
    uint32_t i, j, nels;



        t->buffer = NULL;
        assert(posix_memalign(&t->buffer, CACHE_LINE_SIZE, STEP*sizeof(uint64_t)) == 0);
        assert(t->buffer != NULL);


        // initialize the buffer

        for (j=0; j<STEP; j++)
            t[i].buffer[j] = i;
}

void workload_vsum (thread_data_t *t)
{
    uint64_t i, count;
    uint32_t j;

    count = 0;
	for (i=0; alive; i+=STEP)
    	for (j=0; j<STEP; j++) {
        	count += t->buffer[j];
    	}
	
	
	t->nloops = i;

}


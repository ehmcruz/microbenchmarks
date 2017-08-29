#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

#define STEP 128
#define CACHE_LINE_SIZE 128

typedef enum workload_t {
	WORKLOAD_HARMONIC,
	WORKLOAD_POINTER_CHASING,
	WORKLOAD_NTYPES
} workload_t;

static const char *workload_str_table[] = {
	"H",
	"P"
};

static volatile int alive = 1;
static int walltime;
double total_time;
struct list_el_t {
	struct list_el_t *next;
	uint64_t v;
} __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct list_el_t list_el_t;

struct thread_data_t {
	int id;
	workload_t type;
	uint64_t nloops;
	pid_t tid;	
	int cpu;

	double v;
	uint64_t v2;
	list_el_t *list;
}  __attribute__ ((aligned (CACHE_LINE_SIZE)));

typedef struct thread_data_t thread_data_t;

thread_data_t *threads;
uint32_t nt;

double GetTime(void)
{
   struct  timeval time;
   double  Time;

   gettimeofday(&time, (struct timezone *) NULL);
   Time = ((double)time.tv_sec*1000000.0 + (double)time.tv_usec);
   return(Time);
}

void libmapping_set_aff_thread (pid_t pid, uint32_t cpu)
{
#if defined(linux) || defined (__linux)
    int ret;
    cpu_set_t mask;

#if defined(LIBMAPPING_MODE_SIMICS)
    if (cpu >= 2)
        cpu += 2; // in simics, for whatever reason, cpus 2 and 3 do not exist
#endif

    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    errno = 0;
    ret = sched_setaffinity(pid, sizeof(mask), &mask);
    if (errno != 0) {
        printf("libmapping error, cannot bind to CPU %u\n", cpu);
        switch (errno) {
            case EFAULT:
                printf("error: EFAULT\n");
                break;
            case EINVAL:
                printf("error: EINVAL\n");
                break;
            case EPERM:
                printf("error: EPERM\n");
                break;
            case ESRCH:
                printf("error: ESRCH\n");
                break;
            default:
                printf("error: unknown\n");
                break;
        }
/*      LM_ASSERT(0)*/
    }

    //LM_ASSERT_PRINTF(!(ret == -1 && errno != 0), "cannot bind thread %u to PU %u\n", (uint32_t)pid, cpu)
    //LM_ASSERT_PRINTF(!(ret == -1), "cannot bind thread %u to PU %u\n", (uint32_t)pid, cpu)
#else
    #error Only linux is supported at the moment
#endif
}

static char* my_strtok (char *str, char *tok, char del, uint32_t bsize)
{
	char *p;
	uint32_t i;

	for (p=str, i=0; *p && *p != del; p++, i++) {
		assert(i < (bsize-1));
		*tok = *p;
		tok++;
	}

	*tok = 0;
	
	if (*p)
		return p + 1;
	else if (p != str)
		return p;
	else
		return NULL;
}

static void parse_affinity (char *str)
{
	char tok[32], *p;
	uint32_t i;
	int *vec;
	
	p = str;
	int n = 1;
	for (p=str; *p; p++)
		n += (*p == ',');
	
	vec = malloc(n * sizeof(int));
	assert(vec != NULL);
	
	p = str;
	p = my_strtok(p, tok, ',', 32);
	i = 0;
	
	while (p != NULL) {
		assert(i < n);
		vec[i] = atoi(tok);	
		
		i++;
		
		p = my_strtok(p, tok, ',', 32);
	}
	
	assert(i == n);
	assert(n == nt);
	
	for (i=0; i<nt; i++)
		threads[i].cpu = vec[i];
	free(vec);
}
static void parse_type_vector (char *str)
{
	char tok[32], *p;
	uint32_t i;
	workload_t *vec;
	
	p = str;
	int n = 1;
	for (p=str; *p; p++)
		n += (*p == ',');
	
	vec = malloc(n * sizeof(workload_t));
	assert(vec != NULL);
	
	p = str;
	p = my_strtok(p, tok, ',', 32);
	i = 0;
	
	while (p != NULL) {
		assert(i < n);
		
		if (!strcmp(tok, "h"))
			vec[i] = WORKLOAD_HARMONIC;
		else if (!strcmp(tok, "p"))
			vec[i] = WORKLOAD_POINTER_CHASING;
		else {
			printf("unknown workload type %s\n", tok);
			exit(1);
		}
		
		i++;
		
		p = my_strtok(p, tok, ',', 32);
	}
	
	assert(i == n);
	
	for (i=0; i<nt; i++)
		threads[i].type = vec[i % n];
	free(vec);
}

static void workload_harmonic (thread_data_t *t)
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

static void workload_pointer_chasing_init_buffer (thread_data_t *t, uint32_t buffer_size)
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
	int	i=1; //só pra compilar	
	el = t->list;
	do {
		el->v = i;
		el = el->next;
		i++; //só pra compilar
	} while (el != t->list);
}

static void workload_pointer_chasing (thread_data_t *t)
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

static void* pthreads_callback (void *data)
{
	thread_data_t *t = (thread_data_t*)data;
	t->tid = syscall(__NR_gettid);	
	libmapping_set_aff_thread(t->tid, t->cpu);
	switch (t->type) {
		case WORKLOAD_HARMONIC:
			workload_harmonic(t);
		break;
		
		case WORKLOAD_POINTER_CHASING:
			workload_pointer_chasing(t);
		break;
		
		default:
			printf("wrong type %i\n", t->type);
			exit(1);
	}
}

static void* time_monitor ()
{
	static double beggin, finish;
	beggin = GetTime();
	usleep(walltime*1000000);
	finish = GetTime();
	total_time = finish - beggin;
	alive = 0;
	return NULL;
}

int main (int argc, char **argv)
{
	pthread_t *ts;

	if(getenv("OMP_NUM_THREADS"))
		nt = atoi(getenv("OMP_NUM_THREADS"));
	else
		goto ERRO;
	if (nt == 0)
		nt = 1;

	ts = malloc(sizeof(pthread_t)*(nt+1));	
	threads = malloc(sizeof(thread_data_t)*nt);	
	if (argc == 3) {
		parse_type_vector(argv[1]);	
		assert(getenv("GOMP_CPU_AFFINITY"));
		parse_affinity(getenv("GOMP_CPU_AFFINITY"));
		walltime=atoi(argv[2]);	
	}
	else
	{	
	ERRO:
		fprintf(stderr, "parametros errados\n"); //placeholder
		exit(-1);
	}
	
	printf("Threads: %d\n", nt);

	for(int i=0; i<nt; i++)
	{
		if(threads[i].type == WORKLOAD_POINTER_CHASING)
			
		printf("%s (%d), ", workload_str_table[threads[i].type], threads[i].cpu);
	}

	for(int i=0; i<nt; i++)
		pthread_create(&ts[i], NULL, pthreads_callback, &threads[i]);

	pthread_create(&ts[nt], NULL, time_monitor, NULL);

	for(int i=0; i<nt; i++)
		pthread_join(ts[i], NULL);
	
	pthread_join(ts[nt], NULL);
		
	printf("total time: %.3f\n", total_time/1000000);
}

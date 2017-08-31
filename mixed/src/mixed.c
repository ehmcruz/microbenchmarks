#include "../headers/workloads.h"

thread_data_t *threads;
uint32_t nt;
volatile int alive = 1;
static int walltime;
double total_time;

static const char *workload_str_table[] = { 
    "H", 
    "P", 
    "V",
    "F"
};

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
		else if (!strcmp(tok, "v"))
			vec[i] = WORKLOAD_VSUM;
		else if (!strcmp(tok, "f"))
			vec[i] = WORKLOAD_FIBONACCI_IT;
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
			
		case WORKLOAD_VSUM:
			workload_vsum(t);	
			break;
		
		case WORKLOAD_FIBONACCI_IT:
			workload_fibonacci_it(t);
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
	uint64_t total_loops[N_WORKLOADS], max_total_loops;
	int i;

	if(getenv("OMP_NUM_THREADS"))
		nt = atoi(getenv("OMP_NUM_THREADS"));
	else {
		printf("missing OMP_NUM_THREADS\n");
		exit(1);
	}
	if (nt == 0)
		nt = 1;

	printf("Threads: %d\n", nt);
	
	ts = malloc(sizeof(pthread_t)*(nt+1));
	assert(ts != NULL);
	
	threads = malloc(sizeof(thread_data_t)*nt);
	assert(threads != NULL);
	
	if (argc == 3) {
		parse_type_vector(argv[1]);	
		assert(getenv("GOMP_CPU_AFFINITY"));
		parse_affinity(getenv("GOMP_CPU_AFFINITY"));
		walltime=atoi(argv[2]);	
	}
	else {	
		printf("usage: %s <type_vector h|p> <time>\n", argv[0]); //placeholder
		exit(-1);
	}

	for(int i=0; i<nt; i++)
	{
		if(threads[i].type == WORKLOAD_POINTER_CHASING)
			workload_pointer_chasing_init_buffer(threads+i, 1024);
		else if(threads[i].type == WORKLOAD_VSUM)
			workload_vsum_init_buffer(&threads[i]);
		printf("%s (%d), ", workload_str_table[threads[i].type], threads[i].cpu);
	}
	printf("\n");

	for(int i=0; i<nt; i++)
		pthread_create(&ts[i], NULL, pthreads_callback, &threads[i]);

	pthread_create(&ts[nt], NULL, time_monitor, NULL);

	for(int i=0; i<nt; i++)
		pthread_join(ts[i], NULL);
	
	pthread_join(ts[nt], NULL);
		
	printf("total time: %.3f\n", total_time/1000000);

	max_total_loops = 0;	
	for (i=0; i<N_WORKLOADS; i++)
		total_loops[i] = 0;
	
	for (i=0; i<nt; i++) {
		total_loops[threads[i].type] += threads[i].nloops;
		max_total_loops += threads[i].nloops;
	}
	for (i=0; i<N_WORKLOADS; i++)
		printf("Total loops %s: %llu\n", workload_str_table[i], total_loops[i]);
	
	printf("Max total loops: %llu\n", max_total_loops);
	
	return 0;
}

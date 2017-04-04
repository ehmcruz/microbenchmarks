CC=gcc
CFLAGS=-O2 -g

all: prodcons-omp prodcons-pthreads

prodcons-omp: prodcons.c
	$(CC) -o $@ $< $(CFLAGS) -fopenmp

prodcons-pthreads: prodcons.c
	$(CC) -o $@ $< $(CFLAGS) -pthread

clean:
	rm -f *.o prodcons-omp prodcons-pthreads

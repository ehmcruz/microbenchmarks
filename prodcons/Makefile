CC=gcc
CPP=g++
CFLAGS=-O2 -ggdb

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: prodcons-omp prodcons-pthread

prodcons-omp: prodcons.c
	$(CC) -o prodcons-omp prodcons.c $(CFLAGS) -fopenmp

prodcons-pthread: prodcons.c
	$(CC) -o prodcons-pthread prodcons.c $(CFLAGS) -lpthread

clean:
	rm -f *.o prodcons-omp prodcons-pthread

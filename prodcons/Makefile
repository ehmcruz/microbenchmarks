CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp -ggdb
LDFLAGS=$(CFLAGS)

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: prodcons.o
	$(CC) -o prodcons prodcons.o $(LDFLAGS)

prodcons.o: prodcons.c
	$(CC) -c prodcons.c $(CFLAGS)

clean:
	rm -f *.o prodcons

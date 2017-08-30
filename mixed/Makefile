CC=gcc
CPP=g++
CFLAGS=-O2 -funroll-loops
LDFLAGS=

#CFLAGS += -fopenmp
LDFLAGS += -lpthread

all: mixed
	@echo "Compiled! Yes!"

mixed: mixed.c workloads.c mixed.h
	$(CC) -o mixed mixed.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o mixed

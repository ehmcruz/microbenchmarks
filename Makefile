CC=gcc
CFLAGS=-O2 -fopenmp
LDFLAGS=-O2 -fopenmp
MAPPINGLIBFLAGS=-DENABLE_OPENMP -DMAPPING_LIB_WITH_PAPI

all: mappinglib
	$(CC) -o full_shared_no_lock full_shared_no_lock.c mapping-lib.o

mappinglib:
	$(CC) ../libmapping/mapping-lib.c -o mapping-lib.o $(MAPPINGLIBFLAGS)

clean:
	- rm *.o
	- rm full_shared_no_lock

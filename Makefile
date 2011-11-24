CC=gcc
CFLAGS=-O2 -fopenmp
LDFLAGS=mapping-lib.o $(CFLAGS)
MAPPINGLIBFLAGS=-DENABLE_OPENMP -DMAPPING_LIB_WITH_PAPI -I../libmapping

all: mapping-lib.o full_shared_no_lock
	$(CC) -o full_shared_no_lock full_shared_no_lock.o $(LDFLAGS)

full_shared_no_lock:
	$(CC) -c full_shared_no_lock.c $(CFLAGS) $(MAPPINGLIBFLAGS)

mapping-lib.o:
	$(CC) ../libmapping/mapping-lib.c -o mapping-lib.o $(CFLAGS) $(MAPPINGLIBFLAGS)

clean:
	- rm *.o
	- rm full_shared_no_lock

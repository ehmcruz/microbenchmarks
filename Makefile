CC=gcc
CFLAGS=-O2 -fopenmp $(MOREFLAGS)
LDFLAGS=mapping-lib.o $(CFLAGS) #-lpapi
MAPPINGLIBFLAGS=-DENABLE_OPENMP -I../libmapping #-DLIBMAPPING_WITH_PAPI

all: mapping-lib.o full_shared_no_lock
	$(CC) -o full_shared_no_lock full_shared_no_lock.o $(LDFLAGS)

full_shared_no_lock:
	$(CC) -c full_shared_no_lock.c $(CFLAGS) $(MAPPINGLIBFLAGS)

mapping-lib.o:
	$(CC) -c ../libmapping/libmapping.c -o mapping-lib.o $(CFLAGS) $(MAPPINGLIBFLAGS)

clean:
	- rm *.o
	- rm full_shared_no_lock
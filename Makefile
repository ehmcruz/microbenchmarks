CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp
LDFLAGS=mapping-lib.o map_algorithm.o libremap.o $(CFLAGS) -lemon -lstdc++ #-lpapi

#MAPFLAGS=-DLIBMAPPING_WITH_PAPI
#MAPFLAGS=-DLIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
MAPFLAGS=-DLIBMAPPING_REMAP_SIMICS_COMM_PATTERN_REALMACHINESIDE
#MAPFLAGS=-DLIBMAPPING_REAL_REMAP_SIMICS

all: mapping-lib.o map_algorithm.o libremap.o full_shared_no_lock full_shared_no_lock_perfect
	$(CC) -o full_shared_no_lock full_shared_no_lock.o $(LDFLAGS)
	$(CC) -o full_shared_no_lock_perfect full_shared_no_lock_perfect.o $(LDFLAGS)

full_shared_no_lock:
	$(CC) -c full_shared_no_lock.c $(CFLAGS) -DENABLE_OPENMP -I../libmapping $(MAPFLAGS)

full_shared_no_lock_perfect:
	$(CC) -c full_shared_no_lock.c -o full_shared_no_lock_perfect.o $(CFLAGS) -DENABLE_OPENMP -I../libmapping -DPERFECT_REMAP $(MAPFLAGS)

mapping-lib.o:
	$(CC) -c ../libmapping/libmapping.c -o mapping-lib.o $(CFLAGS) -DENABLE_OPENMP -I../libmapping $(MAPFLAGS)

libremap.o:
	$(CC) -o libremap.o -c ../libmapping/libremap.c $(CFLAGS) -DENABLE_OPENMP $(MAPFLAGS)

map_algorithm.o:
	$(CPP) -o map_algorithm.o -c ../libmapping/map_algorithm.cpp $(CFLAGS) $(MAPFLAGS)

clean:
	- rm *.o
	- rm full_shared_no_lock

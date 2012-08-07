CC=gcc
CPP=g++
CFLAGS=-O2
LDFLAGS=mapping-lib.o map_algorithm.o libremap.o $(CFLAGS) -lemon -lstdc++

CFLAGS += -fopenmp
#LDFLAGS += -lpthread
#LDFLAGS += -lpapi

LIBMAPPING_PRE_FILTER=2
LIBMAPPING_POS_FILTER=1
LIBMAPPING_MAX_OVERHEAD=0

MAPFLAGS=
MAPFLAGS += -DLIBMAPPING_REMAP_SIMICS_COMM_PATTERN_REALMACHINESIDE
#MAPFLAGS += -DLIBMAPPING_REMAP_SIMICS_COMM_PATTERN_SIMSIDE
#MAPFLAGS += -DLIBMAPPING_REAL_REMAP_SIMICS

MAPFLAGS += -DPRE_FILTER=$(LIBMAPPING_PRE_FILTER) -DPOS_FILTER=$(LIBMAPPING_POS_FILTER) -DMAX_OVERHEAD=$(LIBMAPPING_MAX_OVERHEAD)
#MAPFLAGS += -DDEBUG=0

MAPFLAGS += -DENABLE_OPENMP
#MAPFLAGS += -DENABLE_PTHREADS
#MAPFLAGS += -DLIBMAPPING_WITH_PAPI

all: mapping-lib.o map_algorithm.o libremap.o full_shared_no_lock.o full_shared_no_lock_perfect.o
	$(CC) -o full_shared_no_lock full_shared_no_lock.o $(LDFLAGS)
	$(CC) -o full_shared_no_lock_perfect full_shared_no_lock_perfect.o $(LDFLAGS)

full_shared_no_lock.o:
	$(CC) -c full_shared_no_lock.c $(CFLAGS) -I../libmapping $(MAPFLAGS)

full_shared_no_lock_perfect.o:
	$(CC) -c full_shared_no_lock.c -o full_shared_no_lock_perfect.o $(CFLAGS) -I../libmapping -DPERFECT_REMAP $(MAPFLAGS)

mapping-lib.o:
	$(CC) -c ../libmapping/libmapping.c -o mapping-lib.o $(CFLAGS) -I../libmapping $(MAPFLAGS)

libremap.o:
	$(CC) -o libremap.o -c ../libmapping/libremap.c $(CFLAGS) $(MAPFLAGS)

map_algorithm.o:
	$(CPP) -o map_algorithm.o -c ../libmapping/map_algorithm.cpp $(CFLAGS) $(MAPFLAGS)

clean:
	- rm *.o
	- rm full_shared_no_lock
	- rm full_shared_no_lock_perfect

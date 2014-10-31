CC=gcc
CPP=g++
LMFOLDER=../libmapping
CFLAGS=-O2
LDFLAGS=$(CFLAGS)

CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: full_shared_no_lock.o full_shared_no_lock_perfect.o
	$(CC) -o full_shared_no_lock full_shared_no_lock.o $(LDFLAGS)
	$(CC) -o full_shared_no_lock_perfect full_shared_no_lock_perfect.o $(LDFLAGS) -L$(LMFOLDER) -lmapping

full_shared_no_lock.o: full_shared_no_lock.c
	$(CC) -c full_shared_no_lock.c $(CFLAGS) -I../libmapping

full_shared_no_lock_perfect.o: full_shared_no_lock.c
	$(CC) -c full_shared_no_lock.c -o full_shared_no_lock_perfect.o $(CFLAGS) -I$(LMFOLDER) -DPERFECT_REMAP

clean:
	rm -f *.o full_shared_no_lock full_shared_no_lock_perfect

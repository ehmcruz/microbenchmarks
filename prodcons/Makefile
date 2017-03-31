CC=gcc
CPP=g++
LMFOLDER=../libmapping
CFLAGS=-O2 -ggdb
LDFLAGS=$(CFLAGS)

#CFLAGS += -fopenmp
LDFLAGS += -lpthread

all: full_shared_no_lock.o full_shared_no_lock_perfect.o full_shared_no_lock_dm.o
	$(CC) -o full_shared_no_lock full_shared_no_lock.o -L$(LMFOLDER) -lmapping $(LDFLAGS)
	ln -s full_shared_no_lock fs.y
	ln -s full_shared_no_lock fs.x
	ln -s full_shared_no_lock fs
	$(CC) -o full_shared_no_lock_perfect full_shared_no_lock_perfect.o $(LDFLAGS) -L$(LMFOLDER) -lmapping
	$(CC) -o full_shared_no_lock_dm full_shared_no_lock_dm.o $(LDFLAGS) -L$(LMFOLDER) -lmapping -lnuma
	ln -s full_shared_no_lock_dm fsdm.y
	ln -s full_shared_no_lock_dm fsdm.x
	ln -s full_shared_no_lock_dm fsdm

full_shared_no_lock.o: full_shared_no_lock.c
	$(CC) -c full_shared_no_lock.c $(CFLAGS) -I../libmapping

full_shared_no_lock_perfect.o: full_shared_no_lock.c
	$(CC) -c full_shared_no_lock.c -o full_shared_no_lock_perfect.o $(CFLAGS) -I$(LMFOLDER) -DPERFECT_REMAP

full_shared_no_lock_dm.o: full_shared_no_lock.c
	$(CC) -c full_shared_no_lock.c -o full_shared_no_lock_dm.o $(CFLAGS) -I$(LMFOLDER) -DWITH_DATAMAPPING

clean:
	rm -f *.o full_shared_no_lock full_shared_no_lock_perfect full_shared_no_lock_dm *.x *.y fs fsdm

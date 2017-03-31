CC=gcc
CPP=g++
LMFOLDER=../libmapping
CFLAGS=-O2 -fopenmp -ggdb
LDFLAGS=$(CFLAGS)

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: pi
	@echo "Compiled! Yes!"

pi: pi.o
	$(CC) -o pi pi.o $(LDFLAGS)

pi.o: pi.c
	$(CC) -c pi.c $(CFLAGS) -I../libmapping

clean:
	rm -f *.o pi

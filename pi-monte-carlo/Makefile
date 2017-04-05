CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp -ggdb

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: pi
	@echo "Compiled! Yes!"

pi: pi.c
	$(CC) -o pi pi.c $(CFLAGS)

clean:
	rm -f *.o pi

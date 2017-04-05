CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp -ggdb

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: pointer-chasing
	@echo "Compiled! Yes!"

pointer-chasing: pointer-chasing.c
	$(CC) -o pointer-chasing pointer-chasing.c $(CFLAGS)

clean:
	rm -f pointer-chasing

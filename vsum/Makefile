CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp -g

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: vsum
	@echo "Compiled! Yes!"

vsum: vsum.c
	$(CC) -o vsum vsum.c $(CFLAGS)

clean:
	rm -f vsum

CC=gcc
CPP=g++
CFLAGS=-O2 -fopenmp -g

#CFLAGS += -fopenmp
#LDFLAGS += -lpthread

all: harmonic
	@echo "Compiled! Yes!"

harmonic: harmonic.c
	$(CC) -o harmonic harmonic.c $(CFLAGS)

clean:
	rm -f *.o harmonic

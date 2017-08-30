CC=gcc
CPP=g++
CFLAGS=-O2 -funroll-loops
LDFLAGS=
SRCDIR=src/
#CFLAGS += -fopenmp
LDFLAGS += -lpthread

all: mixed
	@echo "Compiled! Yes!"

mixed: $(SRCDIR)mixed.c 
	$(CC) -c $(SRCDIR)mixed.c $(CFLAGS)
workloads : $(SRCDIR)workloads.c
	$(CC) -c $(SRCDIR)workloads.c $(CFLAGS)
all: 
	make mixed
	make workloads
	$(CC) -o mixed mixed.o workloads.o $(LDFLAGS)

clear: 
	rm -f *.o
clean:
	rm -f *.o mixed

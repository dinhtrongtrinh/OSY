CC  = gcc
CXX = g++

EXTRA_CFLAGS =
CFLAGS   = -Wall $(EXTRA_CFLAGS)
CXXFLAGS = -Wall $(EXTRA_CFLAGS)

TARGETS = forkpipe nsd

NSD_OBJ = nsd_main.o nsd.o nd.o

all: $(TARGETS)

forkpipe: forkpipe.o
	$(CXX) $^ -o $@

nsd: $(NSD_OBJ)
	$(CC) $^ -o $@

nsd_main.o: nsd_main.c nsd.h nd.h
	$(CC) $(CFLAGS) -c nsd_main.c -o nsd_main.o

nsd.o: nsd.c nsd.h
	$(CC) $(CFLAGS) -c nsd.c -o nsd.o

nd.o: nd.c nd.h
	$(CC) $(CFLAGS) -c nd.c -o nd.o

clean:
	rm -f $(TARGETS) *.o


CC      = gcc
CXX     = g++
CFLAGS  = -Wall $(EXTRA_CFLAGS)
CXXFLAGS = -Wall $(EXTRA_CFLAGS)

TARGETS = forkpipe nsd

NSD_OBJ = nsd_main.o nsd.o nd.o

all: $(TARGETS)

forkpipe: forkpipe.o
	$(CXX) $(CXXFLAGS) $^ -o $@

nsd: $(NSD_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

nsd_main.o: nsd_main.c nsd.h nd.h
	$(CC) $(CFLAGS) -c $< -o $@

nsd.o: nsd.c nsd.h
	$(CC) $(CFLAGS) -c $< -o $@

nd.o: nd.c nd.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGETS) *.o

# Makefile pro prod-cons
CXX := g++
SRC := prod-cons.cpp
TARGET := prod-cons

EXTRA_CFLAGS ?= -fsanitize=address -fno-omit-frame-pointer

CXXFLAGS := -Wall -g -O2 -std=c++17 $(EXTRA_CFLAGS)
LDFLAGS := $(EXTRA_CFLAGS)
LDLIBS := -pthread

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET) *.o

CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -O2 -g $(EXTRA_CFLAGS)
TARGET = factory
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)
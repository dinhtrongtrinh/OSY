CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -O2 -g $(EXTRA_CFLAGS)
TARGET = The_ticket_race
SOURCES = main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

zip: 
	zip The_ticket_race.zip *cpp *file

clean:
	rm -f $(TARGET)
	rm -f *.zip
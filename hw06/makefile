

CXX = g++
CXXFLAGS = -m32 -nostdlib -static 
TARGET = hexconv
SOURCES = hexconv.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

zip: 
	zip hw06.zip *c

clean:
	rm -f $(TARGET)
	rm -f *.zip
	rm -f *out
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2

TARGET = example
SOURCES = example.cpp apppac.cpp endpoint.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run-listen: $(TARGET)
	./$(TARGET) listen 0.0.0.0 8000

run-connect: $(TARGET)
	./$(TARGET) connect 127.0.0.1 8000

.PHONY: all clean run-listen run-connect

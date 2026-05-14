# Makefile for ADC TCP Host
# Build on development machine for testing

CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread -O2
TARGET_HOST = host_example
TARGET_ADCS2 = adcs2_example

HOST_SOURCES = adc_host.cpp example_host.cpp
HOST_OBJECTS = $(HOST_SOURCES:.cpp=.o)

ADCS2_SOURCES = adc_host.cpp adcs2.cpp adcs2_example.cpp
ADCS2_OBJECTS = $(ADCS2_SOURCES:.cpp=.o)

all: $(TARGET_HOST) $(TARGET_ADCS2)

$(TARGET_HOST): $(HOST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET_HOST) $(HOST_OBJECTS)

$(TARGET_ADCS2): $(ADCS2_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET_ADCS2) $(ADCS2_OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_HOST) $(TARGET_ADCS2)

test: $(TARGET_HOST)
	./$(TARGET_HOST) test

test_adcs2: $(TARGET_ADCS2)
	./$(TARGET_ADCS2) test

.PHONY: all clean test test_adcs2

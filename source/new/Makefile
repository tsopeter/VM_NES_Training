CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2 -I.

COMP ?= BOTH
TYPE ?= DEFAULT

CLIENT_DEFINES =
ifeq ($(TYPE),SERIAL)
CLIENT_DEFINES += -DSERIAL
endif

COMMON_SOURCES = common/apppac.cpp common/endpoint.cpp

CLIENT_TARGET = app
CLIENT_SOURCES = client/main.cpp client/client.cpp adc/adc.cpp $(COMMON_SOURCES)
CLIENT_OBJECTS = $(patsubst %.cpp,build/client/%.o,$(CLIENT_SOURCES))

HOST_TARGET = host_app
HOST_SOURCES = host/main.cpp host/host.cpp $(COMMON_SOURCES)
HOST_OBJECTS = $(patsubst %.cpp,build/host/%.o,$(HOST_SOURCES))

ifeq ($(COMP),HOST)
TARGETS = $(HOST_TARGET)
else ifeq ($(COMP),CLIENT)
TARGETS = $(CLIENT_TARGET)
else
TARGETS = $(CLIENT_TARGET) $(HOST_TARGET)
endif

all: $(TARGETS)

$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(HOST_TARGET): $(HOST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/client/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CLIENT_DEFINES) -c $< -o $@

build/host/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build
	rm -f $(CLIENT_TARGET) $(HOST_TARGET)

.PHONY: all clean

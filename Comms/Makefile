

CXX = g++
CXXFLAGS = -Wall -std=c++17 -I../source/s3

SRC_CLIENT = client.cpp
SRC_HOST = host.cpp
SRC_IP = ../source/s3/IP.cpp

OBJ_CLIENT = $(SRC_CLIENT:.cpp=.o)
OBJ_HOST = $(SRC_HOST:.cpp=.o)
OBJ_IP = $(SRC_IP:.cpp=.o)

all: client host

client: $(OBJ_CLIENT) $(OBJ_IP)
	$(CXX) $(CXXFLAGS) -o $@ $^

host: $(OBJ_HOST) $(OBJ_IP)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o client host
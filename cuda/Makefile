# Makefile for cuda/main.cpp

CXX := g++
CXXFLAGS := -std=c++17 -O2
CUDA_FLAGS := -DUSE_CUDA

# Path to LibTorch (adjust as needed)
LIBTORCH := /home/petertso/Downloads/libtorch
INCLUDES := -I$(LIBTORCH)/include -I$(LIBTORCH)/include/torch/csrc/api/include
LIBS := -Wl,--no-as-needed -L$(LIBTORCH)/lib -ltorch -ltorch_cuda -lcuda -ltorch_cpu -lc10 -lc10_cuda \
        -Wl,-rpath,$(LIBTORCH)/lib


SRC := main.cpp
TARGET := app

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(CUDA_FLAGS) $(INCLUDES) $< -o $@ $(LIBS)

clean:
	rm -f $(TARGET)
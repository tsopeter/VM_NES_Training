CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
LDFLAGS =

# Source files
SRC = main.cpp ../source/s3/Serial.cpp
# Build directory for object files and binary
BUILD_DIR = build
# Object files in build directory
OBJ = $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(notdir $(SRC)))

# Output binary
TARGET = serial

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../source/s3/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)
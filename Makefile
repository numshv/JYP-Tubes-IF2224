# example: make run ARGS=path_to_file.pas

# Directories
SRC_DIR := src
BIN_DIR := bin
HEADER_DIR := $(SRC_DIR)/header

# Compiler
CXX := g++

# This will automatically search for all *.cpp!
SRCS := $(shell find $(SRC_DIR) -name "*.cpp")
OBJS := $(SRCS:%.cpp=%.o)

# Output
TARGET := $(BIN_DIR)/main

# Default rule
all: $(TARGET)

# Link all to one binary
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) -I$(HEADER_DIR) -c $< -o $@

# Just in case bin doesnt exist...
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET) $(ARGS)

.PHONY: all clean run

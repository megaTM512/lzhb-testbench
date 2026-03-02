# Compiler and flags
CXX := g++
CC  := gcc

CXXFLAGS := -std=c++20 -Wall -Wextra -O3 -march=native -fstrict-aliasing
CFLAGS   := -O3

# Target executable name
TARGET := lzhb-testbench

# Source files
SRCS := lzhb-testbench.cpp lzhb-decode.cpp
MALLOC_SRC := malloc_count.c

# Object files
OBJS := $(SRCS:.cpp=.o)
MALLOC_OBJ := malloc_count.o

# Default rule: build the target
$(TARGET): $(OBJS) $(MALLOC_OBJ)
	$(CXX) $(CXXFLAGS) $(OBJS) $(MALLOC_OBJ) -ldl -o $(TARGET)

# Rule to compile each .cpp into .o
%.o: %.cpp lzhb-decode.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to compile malloc_count.c
$(MALLOC_OBJ): $(MALLOC_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program after building
.PHONY: run
run: $(TARGET)
	./$(TARGET) $(ARGS)

# Clean up build artifacts
.PHONY: clean
clean:
	rm -f $(OBJS) $(MALLOC_OBJ) $(TARGET)

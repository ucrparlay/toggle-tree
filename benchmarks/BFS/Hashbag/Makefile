ROOT := ../../..
CXXFLAGS := -O3 -march=native -std=c++20 -pthread
INCLUDES := -I$(ROOT)/include -I$(ROOT)/benchmark_utils/parlaylib/include -I$(ROOT)/benchmark_utils/graph
TARGET := test
all: $(TARGET)
$(TARGET): test.cpp BFS.h
	g++ $(CXXFLAGS) $(INCLUDES) $< -o $@
clean:
	rm -f $(TARGET)

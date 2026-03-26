ROOT := ../..
CXXFLAGS := -O3 -march=native -std=c++20 -pthread
INCLUDES := -I$(ROOT)/external/parlaylib/include/ -I$(ROOT)/external/ParSet/include/ -I$(ROOT)/external/Graph/include/
TARGET := test
all: $(TARGET)
$(TARGET): test.cpp Dijkstra.h
	g++ $(CXXFLAGS) $(INCLUDES) $< -o $@
clean:
	rm -f $(TARGET)
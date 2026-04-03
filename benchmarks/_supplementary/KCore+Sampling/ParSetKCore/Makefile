ROOT := ../../../..
CXXFLAGS := -O3 -march=native -std=c++20 -pthread
INCLUDES := -I$(ROOT)/include -I$(ROOT)/benchmark_utils/parlaylib/include -I$(ROOT)/benchmarks/_supplementary/KCoreSampling/utils
TARGET := kcore
all: $(TARGET)
$(TARGET): kcore.cpp kcore.h
	g++ $(CXXFLAGS) $(INCLUDES) $< -o $@
clean:
	rm -f $(TARGET)

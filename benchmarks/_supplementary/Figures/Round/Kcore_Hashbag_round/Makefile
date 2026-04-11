ROOT := ../../../../..
CXXFLAGS := -O3 -march=native -std=c++20 -pthread
INCLUDES := -I$(ROOT)/include -I$(ROOT)/benchmark_utils/parlaylib/include
TARGET := test
all: $(TARGET)
$(TARGET): test.cpp KCore.h hashbag.h
	g++ $(CXXFLAGS) $(INCLUDES) $< -o $@
clean:
	rm -f $(TARGET)

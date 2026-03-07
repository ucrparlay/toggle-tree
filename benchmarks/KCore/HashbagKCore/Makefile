SUBMODULE := ../Parallel-KCore
CXXFLAGS := -O3 -march=native -std=c++20 -pthread
INCLUDES := -I$(SUBMODULE)/external/parlaylib/include -I$(SUBMODULE)
TARGET := kcore
all: $(TARGET)
$(TARGET): kcore.cpp kcore.h
	g++ $(CXXFLAGS) $(INCLUDES) $< -o $@
clean:
	rm -f $(TARGET)

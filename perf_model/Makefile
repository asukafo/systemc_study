CXX = g++ 
ifneq ($(TERMUX_VERSION),)
	CXX = clang++
endif

CXXFLAGS = -std=c++17 -Wall -Iinc -Wno-unused-but-set-variable -O2

ifneq ($(DEBUG),1)
	CXXFLAGS += -g
endif

CXXFLAGS += -I$(SYSTEMC_HOME)/include

LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -pthread

TARGET = perf_model

SRCS = perf_model.cpp $(wildcard src/*.cpp)

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJS)

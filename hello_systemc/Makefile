CXX = g++ 
ifneq ($(TERMUX_VERSION),)
	CXX = clang++
endif

CXXFLAGS = -std=c++17 -Wall -Iinc -Wno-unused-but-set-variable

ifneq ($(DEBUG),1)
	CXXFLAGS += -g
endif

CXXFLAGS += -I$(SYSTEMC_HOME)/include

UNAME_S := $(shell uname -s)
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -pthread
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -Wl,-rpath,$(SYSTEMC_HOME)/lib
endif

TARGET = hello_systemc

SRCS = main.cpp $(wildcard src/*.cpp)

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJS)

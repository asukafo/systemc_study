# TLM-2.0 学习项目 Step 1: 最简 b_transport

CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -O2

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifndef SYSTEMC_HOME
$(error SYSTEMC_HOME is not set. Please set SYSTEMC_HOME to your SystemC installation)
endif
endif

INCLUDES = -isystem$(SYSTEMC_HOME)/include
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -lm

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -Wl,-rpath,$(SYSTEMC_HOME)/lib
endif

TARGET = step1

.PHONY: all run clean

all: $(TARGET)

$(TARGET): step1.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)
	@echo "[OK] Build complete: ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) 
	rm -rf step1.dSYM

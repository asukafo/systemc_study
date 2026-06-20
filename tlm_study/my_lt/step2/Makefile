# TLM-2.0 学习项目 Step 2: delay + GP 数据传递

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

TARGET = step2

.PHONY: all run clean

all: $(TARGET)

$(TARGET): step2.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LDFLAGS)
	@echo "[OK] Build complete: ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

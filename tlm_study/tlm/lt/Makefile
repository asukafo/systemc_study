# SystemC TLM-2.0 LT (Loosely-Timed) Example Makefile
# Based on Accellera SystemC official examples

# --- Compiler and flags ---
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -O2

# --- SystemC Home ---
# SYSTEMC_HOME must be set in the environment (skip check for clean)
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifndef SYSTEMC_HOME
$(error SYSTEMC_HOME is not set. Please set SYSTEMC_HOME to your SystemC installation directory)
endif
endif

# --- Include paths ---
# -I../common/inc              : common headers (lt_initiator.h, lt_target.h, memory.h, etc.)
# -I./inc                       : LT-specific headers (lt_top.h, initiator_top.h)
# -I$(SYSTEMC_HOME)/include     : SystemC core headers (systemc, tlm.h, tlm_utils/*)
INCLUDES = -I../common/inc -I./inc -isystem$(SYSTEMC_HOME)/include

# --- Linker flags ---
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -lm

# Platform detection for dynamic library path
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -Wl,-rpath,$(SYSTEMC_HOME)/lib
endif

# --- Source files ---
COMMON_SRCS = $(wildcard ../common/src/*.cpp)
LT_SRCS     = $(wildcard ./src/*.cpp)
SRCS        = $(COMMON_SRCS) $(LT_SRCS)

# --- Object files ---
OBJS = $(SRCS:.cpp=.o)

# --- Target executable (output to tlm/lt/lt) ---
TARGET = ./lt

# --- Run log ---
RUN_LOG = ./run.log

# --- Expected log ---
EXPECTED_LOG = ./results/expected.log

# ============================================================
# Targets
# ============================================================

.PHONY: all run check clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)
	@echo "[OK] Build complete: $(TARGET)"

# Pattern rule: compile .cpp → .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# Compile & run — output to lt/run.log
run: $(TARGET)
	@echo "Running $(TARGET)..."
	$(TARGET) > $(RUN_LOG) 2>&1
	@echo "[OK] Output written to $(RUN_LOG)"

# Run & diff against expected.log
# SystemC outputs a copyright banner that is not in expected.log;
# we filter it out and strip blank lines (same as official build) before comparing.
check: $(TARGET)
	@echo "Running $(TARGET)..."
	$(TARGET) > $(RUN_LOG) 2>&1
	@grep -v -E "SystemC 2\.[0-9]|Copyright \(c\)|ALL RIGHTS RESERVED" $(RUN_LOG) | awk 'NF' > $(RUN_LOG).trimmed
	@awk 'NF' $(EXPECTED_LOG) > $(EXPECTED_LOG).trimmed
	@echo "--- Diff (non-blank lines only) ---"
	@diff $(RUN_LOG).trimmed $(EXPECTED_LOG).trimmed && echo "[PASS] Output matches expected.log" || echo "[FAIL] Output differs from expected.log"
	@rm -f $(RUN_LOG).trimmed $(EXPECTED_LOG).trimmed

# Clean: remove object files and executable
clean:
	rm -f $(OBJS) $(TARGET) $(RUN_LOG)
	@echo "[OK] Clean complete"

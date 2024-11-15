# Compiler and flags
LLVM_CONFIG = llvm-config
CXX = $(shell $(LLVM_CONFIG) --cxx)
CXXFLAGS = -std=c++17 -Wall -Wextra -g
LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

# Instrumentation pass
PASS_PATH = ../LLvmStorePass
PASS_NAME = global_var_counters
PASS_FILE = $(PASS_NAME).so


# LLVM tools
OPT = opt
LLC = llc
CLANG = clang++

# Source and executable
SRC = mealy.cpp
BC = mealy.bc
OBJ = mealy.o
EXEC = mealy_machine

# Targets
all: $(EXEC)

build-pass:
	make -C $(PASS_PATH)

# Generate LLVM IR
$(BC): $(SRC)
	$(CLANG) -emit-llvm -c $(SRC) -o $(BC)

# Instrument the IR with the pass
instrument: $(BC)
	$(OPT) -load-pass-plugin=$(PASS_PATH)/$(PASS_FILE) --passes="$(PASS_NAME)" -o instrumented.bc $(BC)

# Compile the instrumented IR to an executable
$(EXEC): build-pass instrument
	$(CLANG) instrumented.bc -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(BC) instrumented.bc $(OBJ) $(EXEC)

.PHONY: all clean instrument

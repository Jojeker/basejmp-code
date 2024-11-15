# Compiler and flags
LLVM_CONFIG = llvm-config
CXX = clang++
CXXFLAGS = $(shell $(LLVM_CONFIG) --cxxflags --ldflags --system-libs --libs core passes) \
					 -fPIC -shared

# Pass source and output
PASS_SRC = global_var_counters.cpp
PASS_SO = global_var_counters.so

# Target
all: $(PASS_SO)

$(PASS_SO): $(PASS_SRC)
	$(CXX) $(CXXFLAGS) $(PASS_SRC) -o $(PASS_SO) 

clean:
	rm -f $(PASS_SO)

.PHONY: all clean


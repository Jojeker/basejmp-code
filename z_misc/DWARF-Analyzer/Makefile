CXX = clang++
CXXFLAGS = -g -O0 -std=c++17 $(shell llvm-config --cxxflags)
LDFLAGS = $(shell llvm-config --ldflags)
LIBS = $(shell llvm-config --libs core support object) -llldb

SRC = analyze_dwarf.cpp
OUT = analyze_dwarf

OBJ = $(SRC:.cpp=.o)

all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) $(OBJ) -o $(OUT) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT)

.PHONY: all clean


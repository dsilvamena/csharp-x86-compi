CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRC      := $(wildcard *.cpp)
BIN      := compiler

$(BIN): $(SRC) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN) $(FILE) $(ARGS)

clean:
	rm -f $(BIN)
	rm -rf outputs

.PHONY: run clean

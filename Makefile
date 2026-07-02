CXX      ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
SRC      := $(wildcard *.cpp)
BIN      := compiler
INPUTS   := $(sort $(wildcard inputs/*.cs))

$(BIN): $(SRC) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN) $(FILE) $(ARGS)

# Compila todos los inputs/*.cs -> outputs/*.s (nativo, funciona en macOS)
compile-all: $(BIN)
	@for f in $(INPUTS); do echo "== $$f =="; ./$(BIN) $$f; done

# Ensambla y ejecuta todos los outputs/*.s en Docker (gcc:13, linux/amd64)
run-all: compile-all
	docker run --rm --platform linux/amd64 -v "$(PWD)/outputs:/w" gcc:13 bash -c \
	  'cd /w && for s in *.s; do b=$${s%.s}; g++ -no-pie $$s -o $$b && echo "== $$b ==" && ./$$b; done'

# Suite de pruebas: compila, ensambla, ejecuta y compara contra tests/expected/.
# En Linux corre directo; en macOS usa: scripts/docker.sh test
test: $(BIN)
	bash scripts/run_tests.sh

clean:
	rm -f $(BIN)
	rm -rf outputs

.PHONY: run compile-all run-all test clean

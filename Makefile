CXX       ?= g++
CPPFLAGS  := -Iinclude
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2
SRC_DIR   := src
INC_DIR   := include
BUILD_DIR := build
BIN       := $(BUILD_DIR)/compiler
SRC       := $(sort $(wildcard $(SRC_DIR)/*.cpp))
HEADERS   := $(sort $(wildcard $(INC_DIR)/*.h))
INPUTS    := $(sort $(wildcard inputs/*.cs))
VALID_INPUTS := $(filter-out inputs/err_%.cs,$(INPUTS))

$(BIN): $(SRC) $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SRC) -o $(BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(BIN)
	./$(BIN) $(FILE) $(ARGS)

# Compila todos los inputs/*.cs validos -> outputs/*.s (nativo, funciona en macOS)
compile-all: $(BIN)
	@for f in $(VALID_INPUTS); do echo "== $$f =="; ./$(BIN) $$f; done

# Ensambla y ejecuta todos los outputs/*.s en Docker (gcc:13, linux/amd64)
run-all: compile-all
	docker run --rm --platform linux/amd64 -v "$(PWD)/outputs:/w" gcc:13 bash -c \
	  'cd /w && for s in $(notdir $(VALID_INPUTS:.cs=.s)); do b=$${s%.s}; g++ -no-pie $$s -o $$b && echo "== $$b ==" && ./$$b; done'

# Suite de pruebas: compila, ensambla, ejecuta y compara contra tests/expected/.
# En Linux corre directo; en macOS usa: scripts/docker.sh test
test: $(BIN)
	bash scripts/run_tests.sh

clean:
	rm -rf $(BUILD_DIR) outputs

.PHONY: run compile-all run-all test clean

CXX     = g++
CXXFLAGS = -Wall -O2 -Iinclude -std=c++17
BUILDDIR = build

SRC     = src/suffix_array.cpp src/fm_index.cpp src/main.cpp

# --- Main binary ---

all: $(BUILDDIR)/fm_index

$(BUILDDIR)/fm_index: $(SRC) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Usage:
#   make convert INPUT=input.bin INDEX=index.bin
#   make query   INDEX=index.bin PATTERN=101

convert: $(BUILDDIR)/fm_index
	./$(BUILDDIR)/fm_index --convert $(INPUT) $(INDEX)

query: $(BUILDDIR)/fm_index
	./$(BUILDDIR)/fm_index --query $(INDEX) $(PATTERN)

# --- Experiments ---

experiments: $(BUILDDIR)/fm_index
	@echo "Running experiments..."
	# placeholder: python3 experiments/run.py

# --- Tests ---

generate_tests:
	python3 generate_tests.py

run_tests: $(BUILDDIR)/fm_index
	@echo "Running tests..."
	# placeholder: bash tests/run_tests.sh

# --- Cleanup ---

clean:
	rm -rf $(BUILDDIR)

.PHONY: all convert query experiments generate_tests run_tests clean

CXX     = g++
CXXFLAGS = -Wall -O2 -Iinclude -std=c++17
BUILDDIR = build

SRC     = src/suffix_array.cpp src/bwt.cpp src/rank.cpp src/fm_index.cpp src/main.cpp

# --- Main binary ---

all: $(BUILDDIR)/fm_index

$(BUILDDIR)/fm_index: $(SRC) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)


# --- Tests ---

generate_tests:
	python3 experiments/generate_experiment_binaries.py

run_tests: generate_tests | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DDEBUG $(SRC) -o $(BUILDDIR)/fm_index
	python3 experiments/run_experiments.py

# --- Cleanup ---

clean:
	rm -rf $(BUILDDIR)

.PHONY: all generate_tests run_tests clean

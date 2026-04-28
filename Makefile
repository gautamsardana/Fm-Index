CXX      = g++
CXXFLAGS = -Wall -O2 -Iinclude -std=c++17
BUILDDIR = build

SRC = src/suffix_array.cpp src/bwt.cpp src/rank.cpp src/jacobson_rank.cpp src/fm_index.cpp src/main.cpp

all: $(BUILDDIR)/fm_index

$(BUILDDIR)/fm_index: $(SRC) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run_correctness_tests: | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DDEBUG $(SRC) -o $(BUILDDIR)/fm_index
	python3 experiments/run_correctness_tests.py $(if $(JACOBSON),--jacobson)

run_performance_tests: | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DPERF $(SRC) -o $(BUILDDIR)/fm_index
	python3 experiments/run_performance_tests.py

run_dataset_evaluations: $(BUILDDIR)/fm_index
	python3 experiments/run_dataset_evaluations.py

clean:
	rm -rf $(BUILDDIR)

.PHONY: all run_correctness_tests run_performance_tests run_dataset_evaluations clean

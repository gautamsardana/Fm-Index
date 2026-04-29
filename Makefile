CXX      = g++
CXXFLAGS = -Wall -O2 -Iinclude -std=c++17
BUILDDIR = build

SRC = src/suffix_array.cpp src/bwt.cpp src/rank.cpp src/jacobson_rank.cpp src/fm_index.cpp src/main.cpp
BENCH_SRC = src/suffix_array.cpp src/bwt.cpp src/rank.cpp src/jacobson_rank.cpp src/fm_index.cpp

all: $(BUILDDIR)/fm_index

$(BUILDDIR)/fm_index: $(SRC) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/custom_benchmark: src/custom_benchmark.cpp $(BENCH_SRC) | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILDDIR)/sdsl_benchmark: src/sdsl_benchmark.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -Isdsl-lite/include $^ -o $@

run_correctness_tests: | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DDEBUG $(SRC) -o $(BUILDDIR)/fm_index
	python3 experiments/run_correctness_tests.py $(if $(SSA),--ssa $(SSA),) $(if $(JACOBSON),--jacobson,)


run_performance_tests: | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -DPERF $(SRC) -o $(BUILDDIR)/fm_index
	python3 experiments/run_performance_tests.py --label naive
	python3 experiments/run_performance_tests.py --label jacobson --jacobson
	python3 experiments/run_performance_tests.py --label ssa32 --ssa 32
	python3 experiments/run_performance_tests.py --label jacobson_ssa32 --jacobson --ssa 32

clean:
	rm -rf $(BUILDDIR)

.PHONY: all run_correctness_tests run_performance_tests clean

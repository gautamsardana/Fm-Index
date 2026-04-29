# FM-Index — Usage Instructions

## Requirements

- g++ (C++17)
- make
- python3

## Build

```bash
make
```

## Run

```
./build/fm_index [--jacobson] [--ssa <k>] --input <file>
                 [--num-runs <n>] --count|--locate [--pattern-file <file> | <pattern>]
                 --extract <start> <end>
```

- `--jacobson` — Jacobson rank structure (lower space)
- `--ssa <k>` — sparse suffix array, sample every k-th entry (k ≥ 2)
- `--input <file>` — `.txt` (ASCII `0`/`1`) or `.bin` (bit-packed)
- `--count` — count occurrences of pattern
- `--locate` — count + all positions of pattern
- `--extract <start> <end>` — extract bits `[start, end)` from the text
- `--num-runs <n>` — repeat query n times (for benchmarking)
- `--pattern-file <file>` — read pattern from a `.bin` file

### SDSL comparison

Download datasets and generate queries first:

```bash
python3 experiments/pull_datasets.py
python3 experiments/generate_queries.py 50 --output queries_50mb.txt
```

Build and run:

```bash
make build/custom_benchmark
make build/sdsl_benchmark

# our implementation: jacobson + ssa32
build/custom_benchmark experiments/datasets/english_50MB queries_50mb.txt \
    experiments/results/build_custom.csv \
    experiments/results/count_custom.csv \
    experiments/results/locate_custom.csv \
    --jacobson --ssa 32

# sdsl
build/sdsl_benchmark experiments/datasets/english_50MB queries_50mb.txt \
    experiments/results/build_sdsl.csv \
    experiments/results/count_sdsl.csv \
    experiments/results/locate_sdsl.csv
```

CSV columns: `input_size_mb, build_time_ms, peak_memory_kb, index_size_kb` (build) and `input_size_mb, query_size_bits, avg_time_us, avg_result_count` (queries).


### Examples

```bash
./build/fm_index --input input.txt --count 01
./build/fm_index --input input.txt --locate 101
./build/fm_index --jacobson --ssa 32 --input input.txt --locate 101
./build/fm_index --input input.txt --extract 10 20
```

## Tests

```bash
make run_correctness_tests                # naive rank
make run_correctness_tests JACOBSON=1     # Jacobson rank
make run_correctness_tests SSA=32         # sparse SA (k=32)
```

## Performance benchmarks

```bash
make run_performance_tests
```

Runs all four variants (naive, jacobson, ssa32, jacobson_ssa32). Results written to `experiments/results/`.

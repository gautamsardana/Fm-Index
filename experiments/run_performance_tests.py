import subprocess
import time
import os
import tempfile
import random
import sys

BINARY = "./build/fm_index"

def generate_input(n_bits):
    return "".join(random.choice("01") for _ in range(n_bits))

def run_perf(text, pattern):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name
    try:
        cmd = ["/usr/bin/time", "-l", BINARY, "--convert-and-query", fname, pattern]
        start = time.perf_counter()
        result = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
        elapsed = time.perf_counter() - start

        peak_mem_bytes = None
        for line in result.stderr.splitlines():
            if "maximum resident set size" in line.lower():
                peak_mem_bytes = int(line.strip().split()[0])
                break

        return elapsed, peak_mem_bytes
    finally:
        os.unlink(fname)

def fmt_mem(b):
    if b is None: return "N/A"
    if b >= 1024 * 1024: return f"{b / (1024*1024):.2f} MB"
    if b >= 1024: return f"{b / 1024:.2f} KB"
    return f"{b} B"

random.seed(42)

benchmarks = [
    (10**3,  10**1),
    (10**4,  10**1),
    (10**5,  10**2),
    (10**6,  10**2),
    (10**7,  10**3),
]

def expected_time_ms(n):
    # O(n log^2 n) build dominates; rough empirical constant
    import math
    return n * (math.log2(n) ** 2) / 1e6

def expected_mem(n):
    # Peak during build_suffix_array: SA + rank + rank2 + new_rank = 4 * 8n = 32n bytes
    return 32 * n

print(f"{'Input (bits)':<15} {'Pattern (bits)':<16} {'Wall time (ms)':<18} {'Expected time (ms)':<22} {'Peak memory':<15} {'Expected memory'}")
print("-" * 95)

for n_bits, p_bits in benchmarks:
    text    = generate_input(n_bits)
    pattern = generate_input(p_bits)
    elapsed, peak_mem = run_perf(text, pattern)
    print(f"{n_bits:<15} {p_bits:<16} {elapsed*1000:<18.1f} {expected_time_ms(n_bits):<22.1f} {fmt_mem(peak_mem):<15} {fmt_mem(expected_mem(n_bits))}")

sys.exit(0)

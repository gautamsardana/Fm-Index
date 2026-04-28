import subprocess
import random
import sys
import os
import tempfile
import math

BINARY = "./build/fm_index"

def generate_input(n_bits):
    return "".join(random.choice("01") for _ in range(n_bits))

def run_perf(n_bits, p_bits):
    text    = generate_input(n_bits)
    pattern = generate_input(p_bits)

    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name

    try:
        result = subprocess.run(
            [BINARY, "--input", fname, "--locate", pattern],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True
        )
        if result.returncode != 0:
            return None

        metrics = {}
        for line in result.stdout.splitlines():
            if line.startswith("perf build:"):
                for part in line.split():
                    if "cpu_ms=" in part:
                        metrics["build_cpu_ms"] = float(part.split("=")[1])
                    if "peak_rss_kb=" in part:
                        metrics["peak_rss_kb"] = int(part.split("=")[1])
            elif line.startswith("perf locate:"):
                for part in line.split():
                    if "cpu_ms=" in part:
                        metrics["query_cpu_ms"] = float(part.split("=")[1])
                    if "mem_delta_kb=" in part:
                        metrics["query_mem_delta_kb"] = int(part.split("=")[1])
        return metrics
    finally:
        os.unlink(fname)

def expected_build_time_ms(n):
    return n * (math.log2(n) ** 2) / 1e6

def expected_peak_kb(n):
    return (32 * n) // 1024

def fmt_mem(kb):
    if kb is None: return "N/A"
    if kb >= 1024: return f"{kb/1024:.1f} MB"
    return f"{kb} KB"

random.seed(42)

benchmarks = [
    (10**3,  10**1),
    (10**4,  10**1),
    (10**5,  10**2),
    (10**6,  10**2),
    (10**7,  10**3),
]

print(f"{'n (bits)':<12} {'p (bits)':<10} {'build cpu(ms)':<16} {'exp build(ms)':<16} {'query cpu(ms)':<16} {'peak mem':<14} {'exp peak':<14} {'query mem delta'}")
print("-" * 110)

for n_bits, p_bits in benchmarks:
    m = run_perf(n_bits, p_bits)
    if m is None:
        print(f"{n_bits:<12} {p_bits:<10} ERROR")
        continue
    print(f"{n_bits:<12} {p_bits:<10} "
          f"{m.get('build_cpu_ms', 0):<16.2f} "
          f"{expected_build_time_ms(n_bits):<16.1f} "
          f"{m.get('query_cpu_ms', 0):<16.4f} "
          f"{fmt_mem(m.get('peak_rss_kb')):<14} "
          f"{fmt_mem(expected_peak_kb(n_bits)):<14} "
          f"{fmt_mem(m.get('query_mem_delta_kb', 0))}")

sys.exit(0)

import subprocess
import argparse
import random
import sys
import os
import csv
import tempfile
import math

BINARY = "./build/fm_index"
N_RUNS = 3

parser = argparse.ArgumentParser()
parser.add_argument("--label",    default="naive")
parser.add_argument("--jacobson", action="store_true")
parser.add_argument("--ssa",      type=int, default=0)
args = parser.parse_args()

LABEL      = args.label
JACOBSON   = args.jacobson
SSA        = args.ssa

random.seed(42)
os.makedirs("experiments/results", exist_ok=True)

def generate_input(n_bits):
    return "".join(random.choice("01") for _ in range(n_bits))

def write_txt(text):
    f = tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False)
    f.write(text)
    f.close()
    return f.name

def write_bin(bits_str):
    padded = bits_str + '0' * ((8 - len(bits_str) % 8) % 8)
    packed = bytearray()
    for i in range(0, len(padded), 8):
        byte = 0
        for b in padded[i:i+8]:
            byte = (byte << 1) | int(b)
        packed.append(byte)
    f = tempfile.NamedTemporaryFile(mode='wb', suffix='.bin', delete=False)
    f.write(packed)
    f.close()
    return f.name

def parse_perf_output(output):
    build = {}
    all_queries = {}
    for line in output.splitlines():
        if line.startswith("perf build:"):
            for part in line.split():
                if "cpu_ms=" in part:           build["cpu_ms"]           = float(part.split("=")[1])
                if "peak_rss_kb=" in part:      build["peak_rss_kb"]      = int(part.split("=")[1])
                if "post_build_rss_kb=" in part: build["post_build_rss_kb"] = int(part.split("=")[1])
        elif line.startswith("perf queries:"):
            for part in line.split():
                if "cpu_ms=" in part: all_queries["cpu_ms"] = float(part.split("=")[1])
    return build, all_queries

def run_build_and_query(fname, pattern, mode="locate", num_runs=1):
    pf = write_bin(pattern)
    try:
        cmd = [BINARY]
        if JACOBSON: cmd += ["--jacobson"]
        if SSA:      cmd += ["--ssa", str(SSA)]
        cmd += ["--input", fname, "--num-runs", str(num_runs),
                f"--{mode}", "--pattern-file", pf]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        if result.returncode != 0:
            return None, None
        return parse_perf_output(result.stdout)
    finally:
        os.unlink(pf)

def avg(vals):
    return sum(vals) / len(vals) if vals else 0

def write_csv(filepath, rows, fieldnames):
    with open(filepath, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ── Part 1: Build scaling ──────────────────────────────────────────────────────
print(f"=== Build scaling [{LABEL}] ===")
print(f"{'n (bits)':<14} {'cpu (ms)':<18} {'peak mem (KB)':<18} {'post-build (KB)'}")
print("-" * 70)

build_sizes = [10**2, 10**3, 10**4, 10**5, 10**6, 10**7]
build_rows  = []

for n in build_sizes:
    cpu_list, peak_list, post_list = [], [], []
    for _ in range(N_RUNS):
        text  = generate_input(n)
        fname = write_txt(text)
        try:
            build, _ = run_build_and_query(fname, "0" * 8, mode="count", num_runs=1)
            if build:
                cpu_list.append(build.get("cpu_ms", 0))
                peak_list.append(build.get("peak_rss_kb", 0))
                post_list.append(build.get("post_build_rss_kb", 0))
        finally:
            os.unlink(fname)
    cpu  = avg(cpu_list)
    peak = avg(peak_list)
    post = avg(post_list)
    build_rows.append({"n": n, "cpu_ms": round(cpu, 4), "peak_kb": int(peak), "post_build_kb": int(post)})
    print(f"{n:<14} {cpu:<18.2f} {int(peak):<18} {int(post)}")

write_csv(f"experiments/results/build_scaling_{LABEL}.csv", build_rows,
          ["n", "cpu_ms", "peak_kb", "post_build_kb"])
"""
# ── Part 2: Query scaling (count only) ────────────────────────────────────────
N_QUERIES   = 100
QUERY_INPUT = 10**6

# get index memory from build CSV at QUERY_INPUT
def get_index_mem_kb(label, n):
    path = f"experiments/results/build_scaling_{label}.csv"
    if not os.path.exists(path):
        return 0
    with open(path) as f:
        for row in csv.DictReader(f):
            if int(row["n"]) == n:
                return int(row["post_build_kb"])
    return 0

index_mem_kb = get_index_mem_kb(LABEL, QUERY_INPUT)

p = 10
pattern_lengths = []
while p <= QUERY_INPUT:
    pattern_lengths.append(p)
    p *= 10

for mode in ["count"]:
    print(f"\n=== Query {mode} [{LABEL}] ===")
    print(f"{'p (bits)':<14} {'total cpu (ms)':<22} {'index mem (KB)'}")
    print("-" * 50)
    rows = []
    for p in pattern_lengths:
        cpu_list = []
        for _ in range(N_RUNS):
            text    = generate_input(QUERY_INPUT)
            start   = random.randint(0, QUERY_INPUT - p)
            pattern = text[start:start + p]
            fname   = write_txt(text)
            try:
                _, all_q = run_build_and_query(fname, pattern, mode=mode, num_runs=N_QUERIES)
                if all_q:
                    cpu_list.append(all_q.get("cpu_ms", 0))
            finally:
                os.unlink(fname)
        cpu = avg(cpu_list)
        rows.append({"p": p, "cpu_ms": round(cpu, 4), "index_mem_kb": index_mem_kb})
        print(f"{p:<14} {cpu:<22.4f} {index_mem_kb}")

    write_csv(f"experiments/results/query_{mode}_{LABEL}.csv", rows,
              ["p", "cpu_ms", "index_mem_kb"])
"""
"""
# ── Part 3: Memory vs occurrences ─────────────────────────────────────────────
OCC_INPUT   = 10**6
OCC_PATTERN = "0" * 100
occ_counts  = [1, 10, 100, 1000, 10000]

print(f"\n=== Locate memory vs occurrences [{LABEL}] ===")
print(f"{'occ':<12} {'mem delta (KB)'}")
print("-" * 30)

def build_text_with_occurrences(n, pattern, occ):
    text = list(generate_input(n))
    p = len(pattern)
    for _ in range(occ):
        pos = random.randint(0, n - p)
        text[pos:pos+p] = list(pattern)
    return "".join(text)

occ_rows = []
for occ in occ_counts:
    mem_list = []
    for _ in range(N_RUNS):
        text  = build_text_with_occurrences(OCC_INPUT, OCC_PATTERN, occ)
        fname = write_txt(text)
        try:
            _, all_q = run_build_and_query(fname, OCC_PATTERN, mode="locate", num_runs=1)
            if all_q:
                mem_list.append(all_q.get("mem_delta_kb", 0))
        finally:
            os.unlink(fname)
    mem = avg(mem_list)
    occ_rows.append({"occ": occ, "mem_kb": round(mem, 2)})
    print(f"{occ:<12} {mem:.2f}")

write_csv(f"experiments/results/locate_memory_{LABEL}.csv", occ_rows,
          ["occ", "mem_kb"])
"""

print(f"\nResults saved for [{LABEL}]")
sys.exit(0)

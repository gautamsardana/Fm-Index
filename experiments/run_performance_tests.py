import subprocess
import random
import sys
import os
import tempfile
import math

BINARY  = "./build/fm_index"
N_RUNS  = 3  # number of runs to average over

random.seed(42)

def generate_input(n_bits):
    return "".join(random.choice("01") for _ in range(n_bits))

def generate_pattern(p_bits):
    return "".join(random.choice("01") for _ in range(p_bits))

def write_txt(text):
    f = tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False)
    f.write(text)
    f.close()
    return f.name

def write_bin(bits_str):
    # pack bits string into bytes MSB first
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
                if "cpu_ms=" in part:      build["cpu_ms"]      = float(part.split("=")[1])
                if "peak_rss_kb=" in part: build["peak_rss_kb"] = int(part.split("=")[1])
        elif line.startswith("perf queries:"):
            for part in line.split():
                if "cpu_ms=" in part:        all_queries["cpu_ms"]      = float(part.split("=")[1])
                if "mem_delta_kb=" in part:  all_queries["mem_delta_kb"] = int(part.split("=")[1])
                if "n_queries=" in part:     all_queries["n_queries"]    = int(part.split("=")[1])
    return build, all_queries

def run_build_and_query(fname, pattern, mode="locate", num_runs=1):
    pf = write_bin(pattern)
    try:
        cmd = [BINARY, "--input", fname,
               "--num-runs", str(num_runs),
               f"--{mode}", "--pattern-file", pf]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        if result.returncode != 0:
            return None, None
        return parse_perf_output(result.stdout)
    finally:
        os.unlink(pf)

def avg(vals):
    return sum(vals) / len(vals) if vals else 0

def fmt_mem(kb):
    if kb >= 1024: return f"{kb/1024:.1f} MB"
    return f"{kb} KB"

# ── Part 1: Build scaling ──────────────────────────────────────────────────────
print("=" * 70)
print("Part 1: Build scaling")
print("=" * 70)
print(f"{'n (bits)':<14} {'build cpu (ms)':<18} {'peak mem':<14} {'exp peak'}")
print("-" * 60)

build_sizes = [10**3, 10**4, 10**5, 10**6, 10**7]

for n in build_sizes:
    cpu_ms_list, peak_kb_list = [], []
    for _ in range(N_RUNS):
        text  = generate_input(n)
        fname = write_txt(text)
        try:
            build, _ = run_build_and_query(fname, "0" * 8, mode="count", num_runs=1)
            if build:
                cpu_ms_list.append(build.get("cpu_ms", 0))
                peak_kb_list.append(build.get("peak_rss_kb", 0))
        finally:
            os.unlink(fname)
    exp_peak_kb = (32 * n) // 1024
    print(f"{n:<14} {avg(cpu_ms_list):<18.2f} {fmt_mem(int(avg(peak_kb_list))):<14} {fmt_mem(exp_peak_kb)}")

# ── Part 2: Query scaling (n=10^7, 100 queries per pattern size) ───────────────
N_QUERIES   = 100
QUERY_INPUT = 10**6

p = 10
pattern_lengths = []
while p <= 10**6:
    pattern_lengths.append(p)
    p *= 10

for mode in ["count", "locate"]:
    print()
    print("=" * 70)
    print(f"Part 2: Query scaling  n={QUERY_INPUT}  mode={mode}  n_queries={N_QUERIES}")
    print("=" * 70)
    print(f"{'p (bits)':<12} {'total cpu (ms)':<22} {'mem delta'}")
    print("-" * 50)

    results = {p: {"cpu_ms": [], "mem_delta_kb": []} for p in pattern_lengths}

    for p in pattern_lengths:
        for _ in range(N_RUNS):
            text    = generate_input(QUERY_INPUT)
            start   = random.randint(0, QUERY_INPUT - p)
            pattern = text[start:start + p]  # guaranteed to exist in text
            fname   = write_txt(text)
            try:
                build, all_q = run_build_and_query(fname, pattern, mode=mode, num_runs=N_QUERIES)
                if all_q and build:
                    results[p]["cpu_ms"].append(all_q.get("cpu_ms", 0))
                    results[p]["mem_delta_kb"].append(all_q.get("mem_delta_kb", 0))
            finally:
                os.unlink(fname)

    for p in pattern_lengths:
        cpu = avg(results[p]["cpu_ms"])
        mem = avg(results[p]["mem_delta_kb"])
        print(f"{p:<12} {cpu:<22.4f} {fmt_mem(int(mem))}")

# ── Part 3: Memory vs occurrences ─────────────────────────────────────────────
OCC_INPUT   = 10**6
OCC_PATTERN = "0" * 100  # fixed pattern of 100 zeros
occ_counts  = [1, 10, 100, 1000, 10000]

print()
print("=" * 70)
print(f"Part 3: locate memory vs occurrences  n={OCC_INPUT}  m=100")
print("=" * 70)
print(f"{'occ':<12} {'mem delta'}")
print("-" * 30)

def build_text_with_occurrences(n, pattern, occ):
    # start with random text that avoids the pattern
    text = list(generate_input(n))
    p = len(pattern)
    placed = 0
    attempts = 0
    while placed < occ and attempts < occ * 100:
        pos = random.randint(0, n - p)
        text[pos:pos+p] = list(pattern)
        placed += 1
        attempts += 1
    return "".join(text)

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
    print(f"{occ:<12} {fmt_mem(int(avg(mem_list)))}")

sys.exit(0)

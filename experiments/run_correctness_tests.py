import argparse
import subprocess
import random
import sys
import os
import tempfile

BINARY = "./build/fm_index"

parser = argparse.ArgumentParser(description="Run FM-index correctness tests")
parser.add_argument("--ssa", type=int, default=int(os.environ.get("SSA_RATE", "0")),
                    help="Enable sparse suffix array with sampling rate k (k>=2)")
parser.add_argument("--jacobson", action="store_true",
                    help="Run correctness tests using Jacobson rank only")
args, _ = parser.parse_known_args()

SSA_RATE = args.ssa if args.ssa and args.ssa > 1 else 0
run_jacobson = args.jacobson
run_naive = not args.jacobson

passed = 0
failed = 0

def parse_output(output):
    count = int(output.split("count=")[1].split()[0])
    pos_str = output.split("positions=")[1].split("pattern=")[0].strip()
    positions = sorted(int(p) for p in pos_str.split() if p)
    return count, positions

def run_query(text, pattern, jacobson=False):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name
    try:
        cmd = [BINARY]
        if jacobson:
            cmd += ["--jacobson"]
        if SSA_RATE:
            cmd += ["--ssa", str(SSA_RATE)]
        cmd += ["--input", fname, "--locate", pattern]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        if result.returncode != 0:
            return None, None
        return parse_output(result.stdout.strip())
    finally:
        os.unlink(fname)

def brute_count(text, pattern):
    count = 0
    for i in range(len(text) - len(pattern) + 1):
        if text[i:i+len(pattern)] == pattern:
            count += 1
    return count

def brute_locate(text, pattern):
    positions = []
    for i in range(len(text) - len(pattern) + 1):
        if text[i:i+len(pattern)] == pattern:
            positions.append(i)
    return sorted(positions)

def check_one(label, text, pattern, expected_count, expected_positions, jacobson=False):
    global passed, failed
    suffix = " [jacobson]" if jacobson else ""
    count, positions = run_query(text, pattern, jacobson=jacobson)
    if count is None:
        print(f"FAIL [{label}]{suffix} binary error")
        failed += 1
        return
    ok = count == expected_count and positions == sorted(expected_positions)
    if ok:
        print(f"PASS [{label}]{suffix} T={text} P={pattern} count={count}")
        passed += 1
    else:
        print(f"FAIL [{label}]{suffix} T={text} P={pattern}")
        if count != expected_count:
            print(f"       count: expected={expected_count} got={count}")
        if positions != sorted(expected_positions):
            print(f"       positions: expected={sorted(expected_positions)} got={positions}")
        failed += 1

def check(label, text, pattern, expected_count, expected_positions):
    if not text or len(pattern) == 0:
        return
    if run_naive:
        check_one(label, text, pattern, expected_count, expected_positions, jacobson=False)
    if run_jacobson:
        check_one(label, text, pattern, expected_count, expected_positions, jacobson=True)

def run_extract(text, start, end, jacobson=False):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name
    try:
        cmd = [BINARY]
        if jacobson:
            cmd += ["--jacobson"]
        if SSA_RATE:
            cmd += ["--ssa", str(SSA_RATE)]
        cmd += ["--input", fname, "--extract", str(start), str(end)]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        if result.returncode != 0:
            return None
        output = result.stdout.strip()
        if "extract=" not in output:
            return None
        after = output.split("extract=")[1]
        if " start=" in after:
            return after.split(" start=")[0]
        return after.split()[0] if after.strip() else ""
    finally:
        os.unlink(fname)

def check_extract(label, text, start, end):
    global passed, failed
    if start > end or end > len(text):
        return
    expected = text[start:end]
    if run_naive:
        actual = run_extract(text, start, end, jacobson=False)
        if actual == expected:
            print(f"PASS [extract:{label}] T={text} start={start} end={end} extract={actual}")
            passed += 1
        else:
            print(f"FAIL [extract:{label}] T={text} start={start} end={end}")
            print(f"       extract: expected={expected} got={actual}")
            failed += 1
    if run_jacobson:
        actual = run_extract(text, start, end, jacobson=True)
        if actual == expected:
            print(f"PASS [extract:{label}] [jacobson] T={text} start={start} end={end} extract={actual}")
            passed += 1
        else:
            print(f"FAIL [extract:{label}] [jacobson] T={text} start={start} end={end}")
            print(f"       extract: expected={expected} got={actual}")
            failed += 1

# --- Table tests ---
# 72 fixed cases covering known inputs with hand-verified expected counts and positions.
cases = [
    # Single-character texts: match, mismatch, pattern longer than text
    ("single_match_0",      "0",         "0",        1, [0]),
    ("single_mismatch_0",   "0",         "1",        0, []),
    ("pattern_longer_0",    "0",         "00",       0, []),
    ("single_match_1",      "1",         "1",        1, [0]),
    ("single_mismatch_1",   "1",         "0",        0, []),
    ("pattern_longer_1",    "1",         "11",       0, []),
    # All-zeros text: overlapping matches, full match, absent symbol
    ("allzero_single",      "00000",     "0",        5, [0,1,2,3,4]),
    ("allzero_overlap2",    "00000",     "00",       4, [0,1,2,3]),
    ("allzero_overlap3",    "00000",     "000",      3, [0,1,2]),
    ("allzero_full",        "00000",     "00000",    1, [0]),
    ("allzero_absent",      "00000",     "1",        0, []),
    # All-ones text: symmetric cases to all-zeros
    ("allone_single",       "11111",     "1",        5, [0,1,2,3,4]),
    ("allone_overlap2",     "11111",     "11",       4, [0,1,2,3]),
    ("allone_overlap3",     "11111",     "111",      3, [0,1,2]),
    ("allone_full",         "11111",     "11111",    1, [0]),
    ("allone_absent",       "11111",     "0",        0, []),
    # Alternating text: even/odd positions, cross-boundary patterns, absent pairs
    ("alt_0",               "01010101",  "0",        4, [0,2,4,6]),
    ("alt_1",               "01010101",  "1",        4, [1,3,5,7]),
    ("alt_01",              "01010101",  "01",       4, [0,2,4,6]),
    ("alt_10",              "01010101",  "10",       3, [1,3,5]),
    ("alt_010",             "01010101",  "010",      3, [0,2,4]),
    ("alt_101",             "01010101",  "101",      3, [1,3,5]),
    ("alt_full",            "01010101",  "01010101", 1, [0]),
    ("alt_no00",            "01010101",  "00",       0, []),
    ("alt_no11",            "01010101",  "11",       0, []),
    # Mixed runs (00110011): transitions between runs, multi-char patterns
    ("mixed_0",             "00110011",  "0",        4, [0,1,4,5]),
    ("mixed_1",             "00110011",  "1",        4, [2,3,6,7]),
    ("mixed_00",            "00110011",  "00",       2, [0,4]),
    ("mixed_11",            "00110011",  "11",       2, [2,6]),
    ("mixed_01",            "00110011",  "01",       2, [1,5]),
    ("mixed_10",            "00110011",  "10",       1, [3]),
    ("mixed_0011",          "00110011",  "0011",     2, [0,4]),
    ("mixed_1100",          "00110011",  "1100",     1, [2]),
    # Long runs (000111000): run boundaries, cross-run patterns, absent extension
    ("runs_0",              "000111000", "0",        6, [0,1,2,6,7,8]),
    ("runs_1",              "000111000", "1",        3, [3,4,5]),
    ("runs_000",            "000111000", "000",      2, [0,6]),
    ("runs_111",            "000111000", "111",      1, [3]),
    ("runs_01",             "000111000", "01",       1, [2]),
    ("runs_10",             "000111000", "10",       1, [5]),
    ("runs_01110",          "000111000", "01110",    1, [2]),
    ("runs_1111",           "000111000", "1111",     0, []),
    # Heavy-zero text: deeply overlapping patterns, pattern exactly equal to text, pattern one bit longer
    ("heavy_000",           "0000000",   "000",      5, [0,1,2,3,4]),
    ("heavy_0000",          "0000000",   "0000",     4, [0,1,2,3]),
    ("heavy_full",          "0000000",   "0000000",  1, [0]),
    ("heavy_toolong",       "0000000",   "00000000", 0, []),
    # Boundary text (10001): matches at first/last positions, interior run, full match
    ("boundary_1",          "10001",     "1",        2, [0,4]),
    ("boundary_0",          "10001",     "0",        3, [1,2,3]),
    ("boundary_10",         "10001",     "10",       1, [0]),
    ("boundary_01",         "10001",     "01",       1, [3]),
    ("boundary_000",        "10001",     "000",      1, [1]),
    ("boundary_full",       "10001",     "10001",    1, [0]),
    ("boundary_no11",       "10001",     "11",       0, []),
    # Absent patterns: patterns that cannot appear in the text
    ("absent_111",          "010101",    "111",      0, []),
    ("absent_000",          "010101",    "000",      0, []),
    ("partial_1010",        "010101",    "1010",     1, [1]),
    # Longer random text: varied pattern lengths and positions
    ("random_0",            "011010011001", "0",     6, [0,3,5,6,9,10]),
    ("random_1",            "011010011001", "1",     6, [1,2,4,7,8,11]),
    ("random_01",           "011010011001", "01",    4, [0,3,6,10]),
    ("random_10",           "011010011001", "10",    3, [2,4,8]),
    ("random_110",          "011010011001", "110",   2, [1,7]),
    ("random_001",          "011010011001", "001",   2, [5,9]),
    ("random_0110",         "011010011001", "0110",  2, [0,6]),
    ("random_no111",        "011010011001", "111",   0, []),
    ("random_full",         "011010011001", "011010011001", 1, [0]),
    # Edge cases: match not at position 0, match at start/end, isolated symbol, pattern spanning byte boundary
    ("nowrap_101",          "0101",      "101",      1, [1]),
    ("end_match",           "001",       "01",       1, [1]),
    ("start_match",         "100",       "10",       1, [0]),
    ("isolated_1",          "0001000",   "1",        1, [3]),
    ("suffix_match",        "00101",     "101",      1, [2]),
    ("double_boundary",     "01000010", "010",     2, [0,5]),
    ("double_boundary_2",   "01000010", "0100",    1, [0]),
    ("allzero_long",        "0000000000", "0000",  7, [0,1,2,3,4,5,6]),
    ("allone_long",         "1111111111", "1111",  7, [0,1,2,3,4,5,6]),
    ("rare_one",            "000010000", "1",     1, [4]),
    ("rare_one_absent",     "000010000", "11",    0, []),
]

for args in cases:
    check(*args)

# --- Extract tests ---
# 29 cases verifying extract_substring returns the correct bit string for [start, end).
# Covers: full text, prefix, suffix, middle slice, single bit, empty range,
# run boundaries, last/first bit, alternating slices, and 10 random cases.
extract_cases = [
    ("full", "010101", 0, 6),        # entire text
    ("prefix", "010101", 0, 3),      # first half
    ("suffix", "010101", 3, 6),      # second half
    ("middle", "011010011001", 2, 7), # interior slice
    ("single", "1111", 2, 3),        # single bit
    ("empty", "10101", 2, 2),        # zero-length range
    ("zero_start", "000111000", 0, 4),
    ("one_run", "111000111", 3, 6),  # slice spanning a run of zeros
    ("end_boundary", "010011", 4, 6),
    ("full_long", "011010011001", 0, 12),
    ("tail_one", "100000", 5, 6),    # last bit
    ("head_one", "100000", 0, 1),    # first bit
    ("alt_slice", "0101010101", 1, 9),
    ("runs_slice", "000011110000", 3, 9),
    ("end_full", "0010110", 0, 7),
    ("end_last", "0010110", 6, 7),
    ("start_last", "0010110", 5, 6),
    ("middle_long", "010011001110", 4, 10),
    ("alt_small", "0101", 1, 3),
]

# 10 random extract cases with random text, start, and end
random.seed(7)
for i in range(10):
    n = random.randint(6, 20)
    text = "".join(random.choice("01") for _ in range(n))
    start = random.randint(0, n - 1)
    end = random.randint(start, n)
    extract_cases.append((f"rand_{i}", text, start, end))

for label, text, start, end in extract_cases:
    check_extract(label, text, start, end)

# --- SSA-specific tests (only run when --ssa is passed) ---
# 6 extra cases on a 30-bit text that exercises sparse SA locate and extract:
# 3 extract slices (full, middle, tail) and 3 locate patterns with multiple occurrences.
if SSA_RATE:
    ssa_text = "010101001111000010101001111000"
    ssa_extracts = [
        ("ssa_full", ssa_text, 0, len(ssa_text)),
        ("ssa_mid", ssa_text, 5, 20),
        ("ssa_tail", ssa_text, len(ssa_text) - 8, len(ssa_text)),
    ]
    for label, text, start, end in ssa_extracts:
        check_extract(label, text, start, end)
    ssa_patterns = [
        ("ssa_pat_0", ssa_text, "0101"),
        ("ssa_pat_1", ssa_text, "1111"),
        ("ssa_pat_2", ssa_text, "00001"),
    ]
    for label, text, pattern in ssa_patterns:
        expected_count = brute_count(text, pattern)
        expected_positions = brute_locate(text, pattern)
        check(label, text, pattern, expected_count, expected_positions)

# --- Fuzz tests ---
# 100 random (text, pattern) pairs with text length 1-32 and pattern length 1-8.
# Expected count and positions are computed by brute force and compared against the index.
print("\n--- Fuzz tests ---")
random.seed(42)
for i in range(100):
    n = random.randint(1, 32)
    text = "".join(random.choice("01") for _ in range(n))
    p = random.randint(1, min(n, 8))
    pattern = "".join(random.choice("01") for _ in range(p))
    expected_count = brute_count(text, pattern)
    expected_positions = brute_locate(text, pattern)
    check(f"fuzz_{i}", text, pattern, expected_count, expected_positions)

# --- Bin file test ---
# Verifies that a bit-packed .bin input file is read correctly end-to-end.
# Uses the bit string 01010101 packed into one byte; expects locate("01") = [0,2,4,6].
print("\n--- Bin file test ---")

def run_query_bin(bits, pattern_str, jacobson=False):
    padded = bits + [0] * ((8 - len(bits) % 8) % 8)
    packed = bytearray()
    for i in range(0, len(padded), 8):
        byte = 0
        for b in padded[i:i+8]:
            byte = (byte << 1) | b
        packed.append(byte)
    with tempfile.NamedTemporaryFile(suffix='.bin', delete=False) as f:
        f.write(packed)
        fname = f.name
    try:
        cmd = [BINARY]
        if jacobson:
            cmd += ["--jacobson"]
        if SSA_RATE:
            cmd += ["--ssa", str(SSA_RATE)]
        cmd += ["--input", fname, "--locate", pattern_str]
        result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        if result.returncode != 0:
            return None, None
        return parse_output(result.stdout.strip())
    finally:
        os.unlink(fname)

bits = [0,1,0,1,0,1,0,1]
expected_count, expected_positions = 4, [0,2,4,6]
for jacobson in ([True] if run_jacobson else [False]):
    suffix = " [jacobson]" if jacobson else ""
    count, positions = run_query_bin(bits, "01", jacobson=jacobson)
    if count == expected_count and positions == expected_positions:
        print(f"PASS [bin_input]{suffix} count={count} positions={positions}")
        passed += 1
    else:
        print(f"FAIL [bin_input]{suffix} expected count={expected_count} positions={expected_positions} got count={count} positions={positions}")
        failed += 1

print(f"\n{passed} passed, {failed} failed")
sys.exit(0 if failed == 0 else 1)

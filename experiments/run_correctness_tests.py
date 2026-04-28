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
args, _ = parser.parse_known_args()

SSA_RATE = args.ssa if args.ssa and args.ssa > 1 else 0

passed = 0
failed = 0

def parse_output(output):
    count = int(output.split("count=")[1].split()[0])
    pos_str = output.split("positions=")[1].split("pattern=")[0].strip()
    positions = sorted(int(p) for p in pos_str.split() if p)
    return count, positions

def run_query(text, pattern):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name
    try:
        cmd = [BINARY]
        if SSA_RATE:
            cmd += ["--ssa", str(SSA_RATE)]
        cmd += ["--input", fname, "--locate", pattern]

        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True
        )
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

def check(label, text, pattern, expected_count, expected_positions):
    global passed, failed
    if not text:
        return  # skip empty text (unsupported)
    if len(pattern) == 0:
        return  # skip empty pattern (unsupported)
    count, positions = run_query(text, pattern)
    if count is None:
        print(f"FAIL [{label}] binary error")
        failed += 1
        return
    ok = count == expected_count and positions == sorted(expected_positions)
    if ok:
        print(f"PASS [{label}] T={text} P={pattern} count={count}")
        passed += 1
    else:
        print(f"FAIL [{label}] T={text} P={pattern}")
        if count != expected_count:
            print(f"       count: expected={expected_count} got={count}")
        if positions != sorted(expected_positions):
            print(f"       positions: expected={sorted(expected_positions)} got={positions}")
        failed += 1

# --- Table tests ---
cases = [
    ("single_match_0",      "0",         "0",        1, [0]),
    ("single_mismatch_0",   "0",         "1",        0, []),
    ("pattern_longer_0",    "0",         "00",       0, []),
    ("single_match_1",      "1",         "1",        1, [0]),
    ("single_mismatch_1",   "1",         "0",        0, []),
    ("pattern_longer_1",    "1",         "11",       0, []),
    ("allzero_single",      "00000",     "0",        5, [0,1,2,3,4]),
    ("allzero_overlap2",    "00000",     "00",       4, [0,1,2,3]),
    ("allzero_overlap3",    "00000",     "000",      3, [0,1,2]),
    ("allzero_full",        "00000",     "00000",    1, [0]),
    ("allzero_absent",      "00000",     "1",        0, []),
    ("allone_single",       "11111",     "1",        5, [0,1,2,3,4]),
    ("allone_overlap2",     "11111",     "11",       4, [0,1,2,3]),
    ("allone_overlap3",     "11111",     "111",      3, [0,1,2]),
    ("allone_full",         "11111",     "11111",    1, [0]),
    ("allone_absent",       "11111",     "0",        0, []),
    ("alt_0",               "01010101",  "0",        4, [0,2,4,6]),
    ("alt_1",               "01010101",  "1",        4, [1,3,5,7]),
    ("alt_01",              "01010101",  "01",       4, [0,2,4,6]),
    ("alt_10",              "01010101",  "10",       3, [1,3,5]),
    ("alt_010",             "01010101",  "010",      3, [0,2,4]),
    ("alt_101",             "01010101",  "101",      3, [1,3,5]),
    ("alt_full",            "01010101",  "01010101", 1, [0]),
    ("alt_no00",            "01010101",  "00",       0, []),
    ("alt_no11",            "01010101",  "11",       0, []),
    ("mixed_0",             "00110011",  "0",        4, [0,1,4,5]),
    ("mixed_1",             "00110011",  "1",        4, [2,3,6,7]),
    ("mixed_00",            "00110011",  "00",       2, [0,4]),
    ("mixed_11",            "00110011",  "11",       2, [2,6]),
    ("mixed_01",            "00110011",  "01",       2, [1,5]),
    ("mixed_10",            "00110011",  "10",       1, [3]),
    ("mixed_0011",          "00110011",  "0011",     2, [0,4]),
    ("mixed_1100",          "00110011",  "1100",     1, [2]),
    ("runs_0",              "000111000", "0",        6, [0,1,2,6,7,8]),
    ("runs_1",              "000111000", "1",        3, [3,4,5]),
    ("runs_000",            "000111000", "000",      2, [0,6]),
    ("runs_111",            "000111000", "111",      1, [3]),
    ("runs_01",             "000111000", "01",       1, [2]),
    ("runs_10",             "000111000", "10",       1, [5]),
    ("runs_01110",          "000111000", "01110",    1, [2]),
    ("runs_1111",           "000111000", "1111",     0, []),
    ("heavy_000",           "0000000",   "000",      5, [0,1,2,3,4]),
    ("heavy_0000",          "0000000",   "0000",     4, [0,1,2,3]),
    ("heavy_full",          "0000000",   "0000000",  1, [0]),
    ("heavy_toolong",       "0000000",   "00000000", 0, []),
    ("boundary_1",          "10001",     "1",        2, [0,4]),
    ("boundary_0",          "10001",     "0",        3, [1,2,3]),
    ("boundary_10",         "10001",     "10",       1, [0]),
    ("boundary_01",         "10001",     "01",       1, [3]),
    ("boundary_000",        "10001",     "000",      1, [1]),
    ("boundary_full",       "10001",     "10001",    1, [0]),
    ("boundary_no11",       "10001",     "11",       0, []),
    ("absent_111",          "010101",    "111",      0, []),
    ("absent_000",          "010101",    "000",      0, []),
    ("partial_1010",        "010101",    "1010",     1, [1]),
    ("random_0",            "011010011001", "0",     6, [0,3,5,6,9,10]),
    ("random_1",            "011010011001", "1",     6, [1,2,4,7,8,11]),
    ("random_01",           "011010011001", "01",    4, [0,3,6,10]),
    ("random_10",           "011010011001", "10",    3, [2,4,8]),
    ("random_110",          "011010011001", "110",   2, [1,7]),
    ("random_001",          "011010011001", "001",   2, [5,9]),
    ("random_0110",         "011010011001", "0110",  2, [0,6]),
    ("random_no111",        "011010011001", "111",   0, []),
    ("random_full",         "011010011001", "011010011001", 1, [0]),
    # extra edge cases
    ("nowrap_101",          "0101",      "101",      1, [1]),
    ("end_match",           "001",       "01",       1, [1]),
    ("start_match",         "100",       "10",       1, [0]),
    ("isolated_1",          "0001000",   "1",        1, [3]),
    ("suffix_match",        "00101",     "101",      1, [2]),
]

for args in cases:
    check(*args)

# --- Fuzz tests ---
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
print("\n--- Bin file test ---")

def run_query_bin(bits, pattern_str):
    # pack bits into bytes MSB first
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
        if SSA_RATE:
            cmd += ["--ssa", str(SSA_RATE)]
        cmd += ["--input", fname, "--locate", pattern_str]

        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True
        )
        if result.returncode != 0:
            return None, None
        return parse_output(result.stdout.strip())
    finally:
        os.unlink(fname)

# test: 01010101 (8 bits, one full byte), pattern=01, expected count=4, positions=[0,2,4,6]
bits = [0,1,0,1,0,1,0,1]
count, positions = run_query_bin(bits, "01")
expected_count, expected_positions = 4, [0,2,4,6]
if count == expected_count and positions == expected_positions:
    print(f"PASS [bin_input] count={count} positions={positions}")
    passed += 1
else:
    print(f"FAIL [bin_input] expected count={expected_count} positions={expected_positions} got count={count} positions={positions}")
    failed += 1

print(f"\n{passed} passed, {failed} failed")
sys.exit(0 if failed == 0 else 1)

import subprocess
import os

# --- Config ---
BINARY    = "./build/fm_index"
INDEX_DIR = "experiments/indices"
INPUT_DIR = "experiments/inputs"

os.makedirs(INDEX_DIR, exist_ok=True)

experiments = [
    # (input_file, index_file, patterns)
    ("sample.bin", f"{INDEX_DIR}/sample.idx", ["101", "010"]),
]

# --- Run ---
for input_file, index_file, patterns in experiments:
    # Step 1: convert
    subprocess.run([BINARY, "--convert", input_file, index_file], check=True)

    # Step 2: query each pattern
    for pattern in patterns:
        result = subprocess.run(
            [BINARY, "--query", index_file, pattern],
            capture_output=True, text=True, check=True
        )
        print(f"Pattern={pattern}: {result.stdout.strip()}")

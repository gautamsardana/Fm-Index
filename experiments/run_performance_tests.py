import subprocess
import time
import os
import tempfile
import random
import sys

BINARY = "./build/fm_index"

def run_query_timed(text, pattern):
    with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
        f.write(text)
        fname = f.name
    try:
        start = time.perf_counter()
        subprocess.run(
            [BINARY, "--convert-and-query", fname, pattern],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        return time.perf_counter() - start
    finally:
        os.unlink(fname)

# TODO: add performance benchmarks
# e.g. measure build time, query time across different input sizes
print("Performance tests not yet implemented.")
sys.exit(0)

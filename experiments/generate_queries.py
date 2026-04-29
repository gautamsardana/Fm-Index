#!/usr/bin/env python3
import argparse
import random

def generate_queries(input_size_mb, output_file):
    random.seed(42)
    input_size_bits = int(input_size_mb * 1024 * 1024 * 8)

    pattern_sizes = []
    size = 100
    while size <= input_size_bits // 100:
        pattern_sizes.append(size)
        size *= 10
    if not pattern_sizes:
        pattern_sizes = [100]

    queries = []
    for pattern_size in pattern_sizes:
        max_offset = input_size_bits - pattern_size
        if max_offset <= 0:
            continue
        for _ in range(100):
            queries.append((random.randint(0, max_offset), pattern_size))

    with open(output_file, 'w') as f:
        for offset, size in queries:
            f.write(f"{offset},{size}\n")

    print(f"Wrote {len(queries)} queries to {output_file}")
    print(f"Pattern sizes: {pattern_sizes}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input_size_mb', type=float)
    parser.add_argument('--output', '-o', default='queries.txt')
    args = parser.parse_args()
    generate_queries(args.input_size_mb, args.output)

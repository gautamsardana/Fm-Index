#!/usr/bin/env python3

import argparse
import random


def generate_queries(input_size_mb, output_file):
    random.seed(42)

    input_size_bits = int(input_size_mb * 1024 * 1024 * 8)

    pattern_sizes = []
    size = 1
    while size < input_size_bits and size <= input_size_bits // 100:
        pattern_sizes.append(size)
        size *= 10

    if not pattern_sizes:
        pattern_sizes = [1]

    print(f"Input size: {input_size_mb} MB = {input_size_bits:,} bits")
    print(f"Pattern sizes: {pattern_sizes}")
    print(f"Queries per size: 100")
    print(f"Total queries: {len(pattern_sizes) * 100}")

    queries = []

    for pattern_size in pattern_sizes:
        max_offset = input_size_bits - pattern_size

        if max_offset <= 0:
            continue

        for _ in range(100):
            offset = random.randint(0, max_offset)
            queries.append((offset, pattern_size))

    with open(output_file, 'w') as f:
        for offset, size in queries:
            f.write(f"{offset},{size}\n")

    print(f"\nWrote {len(queries)} queries to {output_file}")

    print("\nQueries by pattern size:")
    for pattern_size in pattern_sizes:
        count = sum(1 for _, s in queries if s == pattern_size)
        print(f"  Size {pattern_size:>6} bits: {count:>3} queries")


def main():
    parser = argparse.ArgumentParser(
        description='Generate query patterns for FM-index benchmarking'
    )
    parser.add_argument(
        'input_size_mb',
        type=float,
        help='Size of input file in megabytes'
    )
    parser.add_argument(
        '--output',
        '-o',
        default='queries.txt',
        help='Output query file (default: queries.txt)'
    )

    args = parser.parse_args()

    if args.input_size_mb <= 0:
        parser.error("Input size must be positive")

    generate_queries(args.input_size_mb, args.output)


if __name__ == '__main__':
    main()

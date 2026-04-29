import csv
import math
import os
import sys

os.makedirs("experiments/plots", exist_ok=True)

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use("Agg")
except ImportError:
    print("matplotlib not installed")
    sys.exit(1)

VARIANTS = ["naive", "jacobson", "ssa32", "jacobson_ssa32"]

def read_csv(filepath):
    if not os.path.exists(filepath):
        return []
    with open(filepath) as f:
        return list(csv.DictReader(f))

def make_plot(title, xlabel, ylabel, x_col, y_col, files_by_variant,
              theory_fn, theory_label, outfile):
    fig, ax = plt.subplots()
    ref_xs, ref_ys = None, None

    for variant, filepath in files_by_variant:
        rows = read_csv(filepath)
        if not rows:
            continue
        xs = [float(r[x_col]) for r in rows]
        ys = [float(r[y_col]) for r in rows]
        ax.plot(xs, ys, marker='o', label=variant)
        if ref_xs is None:
            ref_xs, ref_ys = xs, ys

    if ref_xs and theory_fn:
        theory_ys = [theory_fn(x) for x in ref_xs]
        if theory_ys[-1] > 0 and ref_ys[-1] > 0:
            c = ref_ys[-1] / theory_ys[-1]
            ax.plot(ref_xs, [c * t for t in theory_ys],
                    linestyle='--', color='gray', label=theory_label)

    ax.set_xscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.legend()
    fig.savefig(outfile, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved {outfile}")

# Build time
make_plot(
    "Build time vs input size", "n (bits)", "CPU time (ms)", "n", "cpu_ms",
    [(v, f"experiments/results/build_scaling_{v}.csv") for v in VARIANTS],
    lambda n: n * (math.log2(n) ** 2), "O(n log²n)",
    "experiments/plots/build_time.png"
)

# Build peak memory
make_plot(
    "Build peak memory vs input size", "n (bits)", "Peak RSS (KB)", "n", "peak_kb",
    [(v, f"experiments/results/build_scaling_{v}.csv") for v in VARIANTS],
    lambda n: 32 * n / 1024, "O(n)",
    "experiments/plots/build_peak_memory.png"
)

# Post-build memory
make_plot(
    "Post-build memory vs input size", "n (bits)", "RSS after build (KB)", "n", "post_build_kb",
    [(v, f"experiments/results/build_scaling_{v}.csv") for v in VARIANTS],
    lambda n: 16 * n / 1024, "O(n)",
    "experiments/plots/build_post_memory.png"
)

# Query count time
make_plot(
    "Count query time vs pattern length", "m (bits)", "CPU time (ms)", "p", "cpu_ms",
    [(v, f"experiments/results/query_count_{v}.csv") for v in VARIANTS],
    lambda m: m, "O(m)",
    "experiments/plots/query_count_time.png"
)

# Query locate time
make_plot(
    "Locate query time vs pattern length", "m (bits)", "CPU time (ms)", "p", "cpu_ms",
    [(v, f"experiments/results/query_locate_{v}.csv") for v in VARIANTS],
    lambda m: m, "O(m)",
    "experiments/plots/query_locate_time.png"
)

# Locate memory vs occurrences
make_plot(
    "Locate memory vs occurrences", "occurrences", "Memory delta (KB)", "occ", "mem_kb",
    [(v, f"experiments/results/locate_memory_{v}.csv") for v in VARIANTS],
    lambda o: o, "O(occ)",
    "experiments/plots/locate_memory_occ.png"
)

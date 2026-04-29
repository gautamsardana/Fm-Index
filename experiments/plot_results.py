import csv
import math
import os

os.makedirs("experiments/plots", exist_ok=True)

try:
    import matplotlib.pyplot as plt
    import matplotlib
    matplotlib.use("Agg")
except ImportError:
    print("matplotlib not installed")
    exit(1)

VARIANTS = ["naive", "jacobson", "ssa32", "jacobson_ssa32"]
COLORS   = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728"]
MARKERS  = ["o", "s", "^", "D"]

def read_csv(filepath):
    if not os.path.exists(filepath):
        return []
    with open(filepath) as f:
        return list(csv.DictReader(f))

def add_theory(ax, xs, fn, label, ref_y, ref_x):
    ys = [fn(x) for x in xs]
    c = ref_y / fn(ref_x) if fn(ref_x) > 0 else 1
    ax.plot(xs, [c * y for y in ys],
            linestyle='--', color='gray', linewidth=1.5, label=label, zorder=2)

# ── Plot 1: Build time ─────────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(9, 6))
ref_xs, ref_ys = None, None
for v, c, m in zip(VARIANTS, COLORS, MARKERS):
    rows = read_csv(f"experiments/results/build_scaling_{v}.csv")
    if not rows: continue
    xs = [float(r["n"]) for r in rows]
    ys = [float(r["cpu_ms"]) for r in rows]
    ax.plot(xs, ys, marker=m, color=c, label=v, linewidth=2, markersize=7)
    if ref_xs is None: ref_xs, ref_ys = xs, ys

if ref_xs:
    add_theory(ax, ref_xs, lambda n: n * math.log2(n)**2,
               "O(n log²n)", ref_ys[-1], ref_xs[-1])

ax.set_xscale("log")
ax.set_xlabel("n (bits)", fontsize=12)
ax.set_ylabel("CPU time (ms)", fontsize=12)
ax.set_title("Build time vs input size", fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, which="both", linestyle=":", alpha=0.5)
fig.tight_layout()
fig.savefig("experiments/plots/build_time.png", dpi=150)
plt.close(fig)
print("Saved build_time.png")

# ── Plot 2: Build peak memory ──────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(9, 6))
ref_xs, ref_ys = None, None
for v, c, m in zip(VARIANTS, COLORS, MARKERS):
    rows = read_csv(f"experiments/results/build_scaling_{v}.csv")
    if not rows: continue
    xs = [float(r["n"]) for r in rows]
    ys = [float(r["peak_kb"]) for r in rows]
    ax.plot(xs, ys, marker=m, color=c, label=v, linewidth=2, markersize=7)
    if ref_xs is None: ref_xs, ref_ys = xs, ys

if ref_xs:
    add_theory(ax, ref_xs, lambda n: n / 1024,
               "O(n)", ref_ys[-1], ref_xs[-1])

ax.set_xscale("log")
ax.set_xlabel("n (bits)", fontsize=12)
ax.set_ylabel("Peak RSS (KB)", fontsize=12)
ax.set_title("Build peak memory vs input size", fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, which="both", linestyle=":", alpha=0.5)
fig.tight_layout()
fig.savefig("experiments/plots/build_peak_memory.png", dpi=150)
plt.close(fig)
print("Saved build_peak_memory.png")

# ── Plot 3: Post-build memory (log-log to show all 4 lines) ───────────────────
fig, ax = plt.subplots(figsize=(9, 6))
theory_configs = {
    "naive":          (lambda n: n / 1024,       "O(n)"),
    "jacobson":       (lambda n: n / 2048,       "O(n/2)"),
    "ssa32":          (lambda n: n / (32 * 1024), "O(n/32)"),
    "jacobson_ssa32": (lambda n: n / (64 * 1024), "O(n/64)"),
}
for v, c, m in zip(VARIANTS, COLORS, MARKERS):
    rows = read_csv(f"experiments/results/build_scaling_{v}.csv")
    if not rows: continue
    xs = [float(r["n"]) for r in rows]
    ys = [float(r["post_build_kb"]) for r in rows]
    ax.plot(xs, ys, marker=m, color=c, label=v, linewidth=2, markersize=7)
    fn, tlabel = theory_configs[v]
    theory_ys = [fn(x) for x in xs]
    if theory_ys[-1] > 0 and ys[-1] > 0:
        c_scale = ys[-1] / theory_ys[-1]
        ax.plot(xs, [c_scale * y for y in theory_ys],
                linestyle='--', color=c, linewidth=1, alpha=0.5)

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("n (bits)", fontsize=12)
ax.set_ylabel("Heap in use (KB)", fontsize=12)
ax.set_title("Post-build memory vs input size", fontsize=13)
ax.legend(fontsize=10)
ax.grid(True, which="both", linestyle=":", alpha=0.5)
fig.tight_layout()
fig.savefig("experiments/plots/build_post_memory.png", dpi=150)
plt.close(fig)
print("Saved build_post_memory.png")

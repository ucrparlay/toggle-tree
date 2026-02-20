import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator


def _read_csv(path):
    rows = []
    with open(path, newline="") as f:
        reader = csv.DictReader(f, skipinitialspace=True)
        for row in reader:
            rows.append(row)
    return rows


def _to_float(values, name):
    out = []
    for i, v in enumerate(values):
        try:
            out.append(float(v))
        except (TypeError, ValueError):
            raise ValueError(f"Invalid numeric value in column '{name}' at row {i}: {v}")
    return out


def _auto_log_scale(ax, values):
    positive = [v for v in values if v > 0]
    if len(positive) < 2:
        return
    vmin = min(positive)
    vmax = max(positive)
    if vmin > 0 and (vmax / vmin) >= 100:
        ax.set_yscale("log")


def _chunk_bounds(n, segments):
    bounds = [round(i * n / segments) for i in range(segments + 1)]
    out = []
    for i in range(segments):
        start = bounds[i]
        end = bounds[i + 1]
        if end <= start:
            end = min(start + 1, n)
        out.append((start, end))
    return out


def _plot_pair(ax_size, ax_time, x, size_values, time_values, xlabel, title=None):
    ax_size.plot(x, size_values, color="tab:blue", linestyle="-", marker=None, linewidth=1.8)
    ax_size.set_ylabel("size")
    if title:
        ax_size.set_title(title, fontsize=11)
    ax_size.grid(True, which="major", alpha=0.35)
    _auto_log_scale(ax_size, size_values)
    ax_size.xaxis.set_major_locator(MaxNLocator(nbins=8))

    ax_time.plot(x, time_values, color="tab:red", linestyle="-", marker=None, linewidth=1.8)
    ax_time.set_xlabel(xlabel)
    ax_time.set_ylabel("time (s)")
    ax_time.grid(True, which="major", alpha=0.35)
    _auto_log_scale(ax_time, time_values)
    ax_time.xaxis.set_major_locator(MaxNLocator(nbins=8))


def _save_segment_figure(path, x, size_values, time_values, xlabel, title):
    fig, (ax_size, ax_time) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
    _plot_pair(ax_size, ax_time, x, size_values, time_values, xlabel, title=title)
    fig.tight_layout()
    fig.savefig(path, dpi=170)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", default="round_timing.csv")
    ap.add_argument("--x", default="index", choices=["index", "k"])
    ap.add_argument("--out", default="peeling_rounds.png")
    ap.add_argument("--segments", type=int, default=1, help="Split rows into N chunks")
    ap.add_argument(
        "--separate-files",
        action="store_true",
        help="When segments > 1, save each chunk as a separate figure instead of one grid",
    )
    args = ap.parse_args()

    rows = _read_csv(args.csv)
    if not rows:
        raise ValueError(f"CSV has no data rows: {args.csv}")

    if "size" not in rows[0]:
        raise KeyError("CSV is missing required column: size")
    if "time" not in rows[0]:
        raise KeyError("CSV is missing required column: time")

    size_values = _to_float([r["size"] for r in rows], "size")
    time_values = _to_float([r["time"] for r in rows], "time")

    if args.x == "k":
        if "k" not in rows[0]:
            raise KeyError("CSV is missing required column for --x k: k")
        x = _to_float([r["k"] for r in rows], "k")
        xlabel = "k"
    else:
        x = list(range(len(rows)))
        xlabel = "row index"

    n = len(rows)
    segments = max(1, args.segments)

    if segments == 1:
        _save_segment_figure(args.out, x, size_values, time_values, xlabel, title="Round Metrics")
        print(f"Wrote {args.out}")
        return

    bounds = _chunk_bounds(n, segments)

    if args.separate_files:
        out_path = Path(args.out)
        out_dir = out_path.parent if str(out_path.parent) != "" else Path(".")
        stem = out_path.stem
        suffix = out_path.suffix or ".png"
        width = max(2, len(str(segments)))

        for i, (start, end) in enumerate(bounds, start=1):
            xs = x[start:end]
            ys = size_values[start:end]
            yt = time_values[start:end]
            chunk_title = f"Chunk {i}: rows {start}-{max(start, end - 1)}"
            out_file = out_dir / f"{stem}_part_{i:0{width}d}{suffix}"
            _save_segment_figure(str(out_file), xs, ys, yt, xlabel, title=chunk_title)
            print(f"Wrote {out_file}")
        return

    fig, axes = plt.subplots(segments, 2, figsize=(14, 2.6 * segments), squeeze=False)
    for i, (start, end) in enumerate(bounds):
        xs = x[start:end]
        ys = size_values[start:end]
        yt = time_values[start:end]

        ax_size = axes[i][0]
        ax_time = axes[i][1]

        ax_size.plot(xs, ys, color="tab:blue", linestyle="-", marker=None, linewidth=1.6)
        ax_size.set_ylabel("size")
        ax_size.grid(True, which="major", alpha=0.30)
        _auto_log_scale(ax_size, ys)
        ax_size.set_title(f"Chunk {i + 1}: rows {start}-{max(start, end - 1)}", fontsize=10)
        ax_size.xaxis.set_major_locator(MaxNLocator(nbins=6))

        ax_time.plot(xs, yt, color="tab:red", linestyle="-", marker=None, linewidth=1.6)
        ax_time.set_ylabel("time (s)")
        ax_time.grid(True, which="major", alpha=0.30)
        _auto_log_scale(ax_time, yt)
        ax_time.set_title(f"Chunk {i + 1}: rows {start}-{max(start, end - 1)}", fontsize=10)
        ax_time.xaxis.set_major_locator(MaxNLocator(nbins=6))

        if i == segments - 1:
            ax_size.set_xlabel(xlabel)
            ax_time.set_xlabel(xlabel)

    fig.suptitle(f"Round Metrics Split Into {segments} Chunks", fontsize=13)
    fig.tight_layout()
    fig.savefig(args.out, dpi=170)
    plt.close(fig)
    print(f"Wrote {args.out}")


if __name__ == "__main__":
    main()

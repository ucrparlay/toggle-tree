#!/usr/bin/env python3

from __future__ import annotations

import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import LogLocator


ROOT = Path(__file__).resolve().parent
OUT_DIR = ROOT / "round"
OUTPUT = OUT_DIR / "round.pdf"
CAPTION = (
    "Per-round time comparison of Parallel Hash Bag and Toggle Tree. "
    "Each point represents one round, with the x-axis showing the frontier size "
    "and the y-axis showing the time. Lines show log-log linear regression fits."
)
X_MAX = 1e6
Y_MAX = 1e-2
FIT_BINS = 16

PANELS = [
    ("KCore EW", ROOT / "Kcore_Hashbag/enwiki-2023_sym_rounds.csv", ROOT / "Kcore_ToT/enwiki-2023_sym_rounds.csv"),
    ("Coloring EW", ROOT / "Coloring_Hashbag/enwiki-2023_sym_rounds.csv", ROOT / "Coloring_ToT/enwiki-2023_sym_rounds.csv"),
    ("BFS FS", ROOT / "BFS_Hashbag/friendster_sym_rounds.csv", ROOT / "BFS_ToT/friendster_sym_rounds.csv"),
    ("KCore IC", ROOT / "Kcore_Hashbag/indochina_sym_rounds.csv", ROOT / "Kcore_ToT/indochina_sym_rounds.csv"),
    ("Coloring IC", ROOT / "Coloring_Hashbag/indochina_sym_rounds.csv", ROOT / "Coloring_ToT/indochina_sym_rounds.csv"),
    ("BFS PL", ROOT / "BFS_Hashbag/planet_sym_rounds.csv", ROOT / "BFS_ToT/planet_sym_rounds.csv"),
]


def load_pair(hashbag_csv: Path, tot_csv: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    with hashbag_csv.open(newline="") as f:
        bag_rows = list(csv.DictReader(f))
    with tot_csv.open(newline="") as f:
        tot_rows = list(csv.DictReader(f))

    if not bag_rows or set(bag_rows[0].keys()) != {"frontier_size", "time"}:
        raise ValueError(f"unexpected columns in {hashbag_csv}")
    if not tot_rows or list(tot_rows[0].keys()) != ["time"]:
        raise ValueError(f"unexpected columns in {tot_csv}")
    if len(bag_rows) != len(tot_rows):
        raise ValueError(f"row count mismatch: {hashbag_csv}={len(bag_rows)} vs {tot_csv}={len(tot_rows)}")

    size = np.array([float(row["frontier_size"]) for row in bag_rows], dtype=float)
    time_bag = np.array([float(row["time"]) for row in bag_rows], dtype=float)
    time_tot = np.array([float(row["time"]) for row in tot_rows], dtype=float)
    return size, time_bag, time_tot


def subsample(size: np.ndarray, y1: np.ndarray, y2: np.ndarray, limit: int = 2000) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    n = len(size)
    if n <= limit:
        return size, y1, y2
    rng = np.random.default_rng(42)
    idx = rng.choice(n, size=limit, replace=False)
    idx.sort()
    return size[idx], y1[idx], y2[idx]


def plot_fit(ax: plt.Axes, x: np.ndarray, y: np.ndarray, color: str, label: str) -> None:
    mask = (x > 0) & (x <= X_MAX) & (y > 0) & (y <= Y_MAX)
    if mask.sum() < 2:
        return
    x_ok = x[mask]
    y_ok = y[mask]

    lx = np.log10(x_ok)
    ly = np.log10(y_ok)
    edges = np.linspace(lx.min(), lx.max(), FIT_BINS + 1)
    bx: list[float] = []
    by: list[float] = []
    for i in range(FIT_BINS):
        if i + 1 == FIT_BINS:
            in_bin = (lx >= edges[i]) & (lx <= edges[i + 1])
        else:
            in_bin = (lx >= edges[i]) & (lx < edges[i + 1])
        if not np.any(in_bin):
            continue
        bx.append(float(np.median(lx[in_bin])))
        by.append(float(np.median(ly[in_bin])))

    if len(bx) < 2:
        return

    coef = np.polyfit(np.array(bx), np.array(by), 1)
    x_fit = np.logspace(np.log10(x_ok.min()), np.log10(x_ok.max()), 100)
    y_fit = 10 ** np.polyval(coef, np.log10(x_fit))
    ax.plot(x_fit, y_fit, color=color, linewidth=1.5, label=label)


def axis_limits(panels_data: list[tuple[np.ndarray, np.ndarray, np.ndarray]]) -> tuple[tuple[float, float], tuple[float, float]]:
    x_all = np.concatenate([size[(size > 0) & (size <= X_MAX)] for size, _, _ in panels_data])
    y_all = np.concatenate(
        [
            series[(size > 0) & (size <= X_MAX) & (series > 0) & (series <= Y_MAX)]
            for size, time_bag, time_tot in panels_data
            for series in (time_bag, time_tot)
        ]
    )
    x_min = 10 ** np.floor(np.log10(x_all.min()))
    x_max = X_MAX
    y_min = 10 ** np.floor(np.log10(y_all.min()))
    y_max = Y_MAX
    return (x_min, x_max), (y_min, y_max)


def draw_one(
    ax: plt.Axes,
    title: str,
    size: np.ndarray,
    time_bag: np.ndarray,
    time_tot: np.ndarray,
    xlim: tuple[float, float],
    ylim: tuple[float, float],
) -> None:
    mask = (size <= X_MAX) & (time_bag <= Y_MAX) & (time_tot <= Y_MAX)
    size = size[mask]
    time_bag = time_bag[mask]
    time_tot = time_tot[mask]
    size_plot, bag_plot, tot_plot = subsample(size, time_bag, time_tot)

    ax.scatter(size_plot, bag_plot, alpha=0.3, s=3, label="Hashbag", color="#7EAAF6")
    ax.scatter(size_plot, tot_plot, alpha=0.3, s=3, label="Toggle Tree", color="#F5C77E")

    plot_fit(ax, size, time_bag, "#3060C0", "Hashbag fit")
    plot_fit(ax, size, time_tot, "#D08000", "Toggle Tree fit")

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlim(*xlim)
    ax.set_ylim(*ylim)
    ax.set_title(title, fontsize=12)
    ax.set_xlabel("Frontier size", fontsize=10)
    ax.xaxis.set_major_locator(LogLocator(base=10))
    ax.yaxis.set_major_locator(LogLocator(base=10))
    ax.tick_params(axis="both", labelsize=9)


def main() -> None:
    OUT_DIR.mkdir(exist_ok=True)
    fig, axes = plt.subplots(2, 3, figsize=(14, 8))
    axes = axes.ravel()
    panels_data = [load_pair(hashbag_csv, tot_csv) for _, hashbag_csv, tot_csv in PANELS]
    xlim, ylim = axis_limits(panels_data)

    for ax, (title, _, _), (size, time_bag, time_tot) in zip(axes, PANELS, panels_data):
        draw_one(ax, title, size, time_bag, time_tot, xlim, ylim)

    axes[0].set_ylabel("Time (s)", fontsize=10)
    axes[3].set_ylabel("Time (s)", fontsize=10)

    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        loc="lower center",
        bbox_to_anchor=(0.5, 0.01),
        ncol=4,
        fontsize=10,
        markerscale=4,
        frameon=False,
    )
    fig.subplots_adjust(hspace=0.28, wspace=0.22, bottom=0.13)
    fig.savefig(OUTPUT, bbox_inches="tight")
    (OUT_DIR / "round.tex").write_text(
        "\\begin{center}\n"
        "\\includegraphics[width=\\textwidth]{experiments/round/round.pdf}\n"
        f"\\captionof{{figure}}{{{CAPTION}}}\n"
        "\\label{fig:round}\n"
        "\\end{center}\n"
    )
    print(f"wrote {OUTPUT}")


if __name__ == "__main__":
    main()

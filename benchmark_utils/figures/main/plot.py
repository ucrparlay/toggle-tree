import csv
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.lines import Line2D
from matplotlib.patches import Rectangle

MAIN_DIR = Path(__file__).resolve().parent / "main"
CSV_PATH = MAIN_DIR / "table.csv"
OUTPUT_DIR = MAIN_DIR

ABBREV = {
    "twitter_sym": "TW",
    "friendster_sym": "FS",
    "com-orkut_sym": "OK",
    "soc-LiveJournal1_sym": "LJ",
    "socfb-konect_sym": "FK",
    "socfb-uci-uni_sym": "FU",
    "eu-2015-host_sym": "EH",
    "clueweb_sym": "CW",
    "sd_arc_sym": "SD",
    "enwiki-2023_sym": "EW",
    "arabic_sym": "AR",
    "uk-2002_sym": "UK",
    "indochina_sym": "IC",
    "GeoLifeNoScale_10_sym": "GL10",
    "CHEM_5_sym": "CH5",
    "Cosmo50_5_sym": "COS5",
    "planet_sym": "PL",
    "europe_sym": "EU",
    "asia_sym": "AS",
    "north-america_sym": "NA",
    "africa_sym": "AF",
    "RoadUSA_sym": "RU",
    "Germany_sym": "DE",
    "hugebubbles-00020_sym": "BBL",
    "hugetrace-00020_sym": "TRCE",
}

METHOD_COLORS = {
    "GBBS": "#7EAAF6",
    "PASGAL": "#F6BD60",
    "ToT(IS)": "#FCC4E2",
}

DASHED_REFERENCE_COLOR = "red"
DASHED_REFERENCE_STYLE = "--"
DASHED_REFERENCE_WIDTH = 3
LEGEND_FONT_SIZE = 26

ALGO_DEFS = [
    {
        "name": "BellmanFord",
        "baseline_methods": [
            {"name": "GBBS", "legend_style": "solid"},
        ],
        "ours_method": {"name": "ToT", "legend_style": "dashed"},
        "caption": "",
        "label": "",
    },
    {
        "name": "BFS",
        "baseline_methods": [
            {"name": "GBBS", "legend_style": "solid"},
            {"name": "PASGAL", "legend_style": "solid"},
        ],
        "ours_method": {"name": "ToT+VGC", "legend_style": "dashed"},
        "caption": "",
        "label": "",
    },
    {
        "name": "Coloring",
        "baseline_methods": [
            {"name": "GBBS", "legend_style": "solid"},
        ],
        "ours_method": {"name": "ToT", "legend_style": "dashed"},
        "caption": "",
        "label": "",
    },
    {
        "name": "KCore",
        "baseline_methods": [
            {"name": "GBBS", "legend_style": "solid"},
            {"name": "PASGAL", "legend_style": "solid"},
        ],
        "ours_method": {"name": "ToT+VGC+SPL", "legend_style": "dashed"},
        "caption": "End-to-end Comparison. Ours are scaled to 1. ",
        "label": "fig:main_",
    },
    {
        "name": "WeightedBFS",
        "baseline_methods": [
            {"name": "GBBS", "legend_style": "solid"},
            {"name": "ToT(IS)", "legend_style": "solid"},
        ],
        "ours_method": {"name": "ToT(IM)", "legend_style": "dashed"},
        "caption": "End-to-end Comparison on wBFS. ToT(IM) is scaled to 1.",
        "label": "fig:main_WeightedBFS",
    },
]

CAP = 5.0


def safe_float(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return np.nan


def load_table(csv_path):
    with csv_path.open(newline="", encoding="utf-8") as file:
        rows = list(csv.reader(file))

    if len(rows) < 3:
        raise ValueError(f"{csv_path} does not contain enough rows")

    top_header = []
    current = ""
    for value in rows[0]:
        text = value.strip()
        if text:
            current = text
        top_header.append(current)

    sub_header = [value.strip() for value in rows[1]]
    header_pairs = list(zip(top_header, sub_header))

    categories = []
    graphs = []
    data = {pair: [] for pair in header_pairs[2:]}
    current_category = ""

    for row in rows[2:]:
        category = row[0].strip()
        if category:
            current_category = category
        categories.append(current_category)
        graphs.append(row[1].strip())

        for pair, value in zip(header_pairs[2:], row[2:]):
            data[pair].append(safe_float(value.strip()))

    graph_labels = [ABBREV.get(graph, graph) for graph in graphs]
    return data, categories, graphs, graph_labels


def build_category_ranges(categories):
    unique_cats = []
    cat_ranges = []
    prev = None
    start = 0

    for index, category in enumerate(categories):
        if category != prev:
            if prev is not None:
                cat_ranges.append((start, index - 1))
                unique_cats.append(prev)
            prev = category
            start = index

    cat_ranges.append((start, len(categories) - 1))
    unique_cats.append(prev)
    return unique_cats, cat_ranges


def get_series(data, algorithm, method):
    key = (algorithm, method)
    if key not in data:
        raise KeyError(f"Missing column: {algorithm} / {method}")
    return np.array(data[key], dtype=float)


def add_category_guides(ax, cat_ranges, unique_cats):
    for index in range(len(cat_ranges) - 1):
        mid = (cat_ranges[index][1] + cat_ranges[index + 1][0]) / 2
        ax.axvline(x=mid, color="grey", linestyle=":", linewidth=1.5, label="_nolegend_")


def add_category_labels(ax, cat_ranges, unique_cats):
    for (start, end), category in zip(cat_ranges, unique_cats):
        mid = (start + end) / 2
        ax.text(
            mid,
            0.97,
            category,
            ha="center",
            va="top",
            fontsize=30,
            transform=ax.get_xaxis_transform(),
            fontstyle="italic",
        )


def add_horizontal_guides(ax):
    for y in range(2, 6):
        ax.axhline(y=y, color="black", linestyle="--", linewidth=0.8, alpha=0.12, zorder=0)


def draw_relative_bars(ax, x_base, ours, baseline_methods, series_by_method, cap):
    n_methods = len(baseline_methods)
    bar_width = min(0.8 / max(n_methods, 1), 0.35)
    offsets = np.linspace(
        -(n_methods - 1) / 2 * bar_width,
        (n_methods - 1) / 2 * bar_width,
        n_methods,
    )

    for offset, method_name in zip(offsets, baseline_methods):
        raw_vals = series_by_method[method_name]
        rel = np.where(
            (ours > 0) & ~np.isnan(ours) & ~np.isnan(raw_vals),
            raw_vals / ours,
            np.nan,
        )
        capped = np.array([min(value, cap) if not np.isnan(value) else 0 for value in rel])
        positions = x_base + offset
        bars = ax.bar(
            positions,
            capped,
            bar_width,
            label=method_name,
            color=METHOD_COLORS.get(method_name, "#CCCCCC"),
            edgecolor="black",
            linewidth=0.8,
        )

        for bar, value in zip(bars, rel):
            if np.isnan(value):
                ax.plot(
                    bar.get_x() + bar.get_width() / 2,
                    0.15,
                    "kx",
                    markersize=14,
                    markeredgewidth=2.5,
                    label="_nolegend_",
                )
            elif value > cap:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    cap + 0.05,
                    f"{value:.1f}",
                    ha="center",
                    va="bottom",
                    fontsize=18,
                    color="black",
                    rotation=0,
                )


def build_legend_handles(baseline_methods, ours_method):
    handles = []
    for method in baseline_methods + [ours_method]:
        method_name = method["name"]
        if method["legend_style"] == "solid":
            handles.append(
                Rectangle(
                    (0, 0),
                    1,
                    1,
                    facecolor=METHOD_COLORS[method_name],
                    edgecolor="black",
                    linewidth=2,
                    label=method_name,
                )
            )
        else:
            handles.append(
                Line2D(
                    [0],
                    [0],
                    linestyle=DASHED_REFERENCE_STYLE,
                    linewidth=DASHED_REFERENCE_WIDTH,
                    color=DASHED_REFERENCE_COLOR,
                    label=method_name,
                )
            )
    return handles


def build_tex_lines(pdf_name, caption, label):
    has_caption = bool(caption)
    lines = []
    lines.extend(
        [
            r"\begin{center}",
            rf"\includegraphics[width=\textwidth]{{experiments/main/{pdf_name}}}",
        ]
    )
    if has_caption:
        lines.append(r"\medskip")
        lines.append(rf"\captionof{{figure}}{{{caption}}}")
    if label:
        lines.append(rf"\label{{{label}}}")
    lines.append(r"\end{center}")
    return lines

def save_tex_file(output_dir, pdf_name, caption, label):
    tex_name = Path(pdf_name).with_suffix(".tex")
    tex_lines = build_tex_lines(pdf_name, caption, label)
    tex_path = output_dir / tex_name
    tex_path.write_text("\n".join(tex_lines) + "\n", encoding="utf-8")
    print(f"Saved {tex_name}")


def save_panels(data, graph_labels, unique_cats, cat_ranges):
    n_graphs = len(graph_labels)
    x_base = np.arange(n_graphs, dtype=float)
    for algo_def in ALGO_DEFS:
        algo_name = algo_def["name"]
        baseline_methods = algo_def["baseline_methods"]
        ours_method = algo_def["ours_method"]
        caption = algo_def.get("caption", "")
        label = algo_def.get("label", "")
        baseline_method_names = [method["name"] for method in baseline_methods]
        ours_method_name = ours_method["name"]
        plt.close("all")
        fig, ax = plt.subplots(figsize=(40, 6))
        handles = build_legend_handles(baseline_methods, ours_method)

        ax.legend(
            handles=handles,
            loc="upper right",
            bbox_to_anchor=(1.0, 1.215),
            fontsize=LEGEND_FONT_SIZE,
            frameon=True,
            facecolor="white",
            framealpha=1.0,
            ncol=len(handles),
            handletextpad=0.4,
            columnspacing=0.7,
            borderpad=0.4,
            edgecolor="none",
        )

        series_by_method = {
            method: get_series(data, algo_name, method)
            for method in baseline_method_names + [ours_method_name]
        }
        ours_vals = series_by_method[ours_method_name]

        draw_relative_bars(ax, x_base, ours_vals, baseline_method_names, series_by_method, CAP)
        ax.axhline(y=1, color="red", linestyle="--", linewidth=2, zorder=0, alpha=0.7)
        add_horizontal_guides(ax)
        add_category_guides(ax, cat_ranges, unique_cats)
        y_max = CAP + 1.5
        ax.set_ylim([0, y_max])
        ax.set_xlim([x_base[0] - 0.6, x_base[-1] + 0.6])
        ax.set_yticks(np.arange(0, CAP + 1, 1))
        ax.tick_params(axis="y", labelsize=24)
        ax.set_title(algo_name, fontsize=34, fontweight="bold", pad=10)
        ax.set_xticks(x_base)
        add_category_labels(ax, cat_ranges, unique_cats)
        ax.set_xticklabels(graph_labels, rotation=0, ha="center", fontsize=24)
        fig.tight_layout()
        fig.subplots_adjust(top=0.75)
        out_name = f"main_{algo_name}.pdf"
        fig.savefig(OUTPUT_DIR / out_name, bbox_inches="tight")
        print(f"Saved {out_name}")
        plt.close(fig)
        save_tex_file(OUTPUT_DIR, out_name, caption, label)


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    data, categories, _graphs, graph_labels = load_table(CSV_PATH)
    unique_cats, cat_ranges = build_category_ranges(categories)
    print(f"Loaded {len(graph_labels)} graphs, {len(unique_cats)} categories from {CSV_PATH.name}")
    save_panels(data, graph_labels, unique_cats, cat_ranges)


if __name__ == "__main__":
    main()

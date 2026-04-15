import argparse
import os
from pathlib import Path
from typing import Dict, List

SCRIPT_DIR = Path(__file__).resolve().parent
os.environ.setdefault("MPLCONFIGDIR", str(SCRIPT_DIR / ".matplotlib"))

import matplotlib.pyplot as plt


CORES = [1, 2, 4, 12, 24, 48, 96, 192]
SINGLE_MARKERS = ["o", "s", "v", "^", "d", "*"]
TASKS = ["kcore", "bfs", "coloring"]
SYSTEM_TASKS = {
    "ToT": ["kcore", "bfs", "coloring"],
    "GBBS": ["kcore", "bfs", "coloring"],
    "PASGAL": ["kcore", "bfs"],
}
GRAPH_LABELS = {
    "friendster_sym": "FS",
    "twitter_sym": "TW",
    "com-orkut_sym": "OK",
    "socfb-konect_sym": "FK",
    "socfb-uci-uni_sym": "FU",
    "soc-LiveJournal1_sym": "LJ",
    "sd_arc_sym": "SD",
    "arabic_sym": "AR",
    "uk-2002_sym": "UK",
    "eu-2015-host_sym": "EH",
    "enwiki-2023_sym": "EW",
    "indochina_sym": "IC",
    "planet_sym": "PL",
    "europe_sym": "EU",
    "asia_sym": "AS",
    "north-america_sym": "NA",
    "us_sym": "US",
    "africa_sym": "AF",
    "RoadUSA_sym": "RU",
    "Germany_sym": "DE",
    "Cosmo50_5_sym": "COS5",
    "GeoLifeNoScale_10_sym": "GL10",
    "hugebubbles-00020_sym": "BBL",
    "hugetrace-00020_sym": "TRCE",
    "CHEM_5_sym": "CH5",
}


def display_graph_name(graph: str) -> str:
    return GRAPH_LABELS.get(graph, graph)

def read_scale_file(file_path: Path) -> Dict[str, List[float]]:
    data: Dict[str, List[float]] = {}

    with file_path.open("r", encoding="utf-8") as file:
        for raw_line in file:
            line = raw_line.strip()
            if not line:
                continue

            words = line.split()
            graph = words[0]
            data[graph] = [float(value) for value in words[1:]]

    return data


def draw_scale_three(all_data: Dict[str, Dict[str, List[float]]], output_path: Path) -> None:
    fig, axes = plt.subplots(1, 3, sharex=True, figsize=(12, 2.5))

    legend_handles = []
    legend_labels = []

    for ax, task in zip(axes, TASKS):
        data = all_data[task]
        for i, (graph, values) in enumerate(data.items()):
            (line,) = ax.plot(
                CORES,
                values,
                label=display_graph_name(graph),
                marker=SINGLE_MARKERS[i % len(SINGLE_MARKERS)],
            )
            if task == TASKS[0]:
                legend_handles.append(line)
                legend_labels.append(display_graph_name(graph))

        ax.set_title(task, fontsize=20, fontstyle="italic")
        ax.set_xscale("log", base=2)
        ax.set_yscale("log", base=2)
        ax.set_xticks([1, 2, 4, 12, 48, 96, 192])
        ax.set_xticklabels([1, 2, 4, 12, 48, 96, "96h"], fontsize=16, rotation=60)
        ax.set_yticks([1, 2, 4, 8, 16, 32, 64, 128])
        ax.set_yticklabels(["1", "2", "4", "8", "16", "32", "64", "128"], fontsize=17)

    fig.legend(
        legend_handles,
        legend_labels,
        loc="upper center",
        bbox_to_anchor=(0.5, 1.2),
        ncol=len(legend_labels),
        fontsize=17,
        frameon=False,
        handletextpad=0.1,
        columnspacing=0.7,
        borderpad=0.4,
    )

    fig.text(0.5, -0.18, "Number of Cores", ha="center", fontsize=20)
    fig.text(0.02, 0.5, "Self Speedup", va="center", rotation="vertical", fontsize=20)
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)


def draw_scale_two(
    data1: Dict[str, List[float]],
    data2: Dict[str, List[float]],
    title1: str,
    title2: str,
    output_path: Path,
) -> None:
    fig, (ax1, ax2) = plt.subplots(1, 2, sharex=True, figsize=(8, 2.5))

    for i, (graph, values) in enumerate(data1.items()):
        ax1.plot(
            CORES,
            values,
            label=display_graph_name(graph),
            marker=SINGLE_MARKERS[i % len(SINGLE_MARKERS)],
            markersize=8,
        )

    for i, (graph, values) in enumerate(data2.items()):
        ax2.plot(
            CORES,
            values,
            label=display_graph_name(graph),
            marker=SINGLE_MARKERS[(i + 4) % len(SINGLE_MARKERS)],
            markersize=8,
        )

    ax1.set_xscale("log", base=2)
    ax1.set_yscale("log", base=2)
    ax2.set_xscale("log", base=2)
    ax2.set_yscale("log", base=2)

    ax1.set_xticks([1, 2, 4, 12, 48, 192])
    ax1.set_xticklabels([1, 2, 4, 12, 48, "96h"], fontsize=16, rotation=0)
    ax1.set_yticks([1, 2, 4, 8, 16, 32, 64, 128])
    ax1.set_yticklabels(["1", "2", "4", "8", "16", "32", "64", "128"], fontsize=17)

    ax2.set_xticks([1, 2, 4, 12, 48, 192])
    ax2.set_xticklabels([1, 2, 4, 12, 48, "96h"], fontsize=16, rotation=0)
    ax2.set_yticks([1, 2, 4, 8, 16, 32, 64, 128])
    ax2.set_yticklabels(["1", "2", "4", "8", "16", "32", "64", "128"], fontsize=17)

    handles, labels = ax1.get_legend_handles_labels()

    ax1.set_title(title1, fontsize=20, fontstyle="italic")
    ax2.set_title(title2, fontsize=20, fontstyle="italic")

    fig.legend(
        handles,
        labels,
        loc="upper center",
        bbox_to_anchor=(0.5, 1.2),
        ncol=6,
        fontsize=17,
        frameon=False,
        handletextpad=0.1,
        columnspacing=0.7,
        borderpad=0.4,
    )

    fig.text(0.5, -0.12, "Number of cores", ha="center", fontsize=20)
    fig.text(0.02, 0.5, "Self speedup", va="center", rotation="vertical", fontsize=20)
    fig.savefig(output_path, bbox_inches="tight")
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate scalability figures from text data.")
    parser.add_argument(
        "--base-dir",
        type=Path,
        default=SCRIPT_DIR,
        help="Directory containing the ToT, GBBS, and PASGAL subdirectories.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=SCRIPT_DIR,
        help="Directory to write the generated figure.",
    )
    parser.add_argument(
        "--output-format",
        choices=["pdf", "png"],
        default="pdf",
        help="Output figure format.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    base_dir = args.base_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    for system, tasks in SYSTEM_TASKS.items():
        data_dir = base_dir / system
        all_data = {}
        for task in tasks:
            input_path = data_dir / f"{task}.txt"
            all_data[task] = read_scale_file(input_path)

        output_path = output_dir / f"{system.lower()}_speedup.{args.output_format}"
        if len(tasks) == 3:
            draw_scale_three(all_data, output_path)
        else:
            draw_scale_two(all_data[tasks[0]], all_data[tasks[1]], tasks[0], tasks[1], output_path)
        print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()

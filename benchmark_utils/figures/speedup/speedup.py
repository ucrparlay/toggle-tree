import argparse
import os
from pathlib import Path
from typing import Dict, List, Optional

SCRIPT_DIR = Path(__file__).resolve().parent
os.environ.setdefault("MPLCONFIGDIR", str(SCRIPT_DIR / ".matplotlib"))
OUT_DIR = SCRIPT_DIR / "speedup"
SPEEDUP_TEX = (
    "\\begin{center}\n"
    "\\includegraphics[width=\\textwidth]{experiments/speedup/speedup.pdf}\n"
    "\\captionof{figure}{Self-Speedup.}\n"
    "\\label{fig:speedup}\n"
    "\\end{center}\n"
)

import matplotlib.pyplot as plt


CORES = [1, 2, 4, 12, 24, 48, 96, 192]
TASKS = ["kcore", "bfs", "coloring"]
SYSTEM_TASKS = {
    "ToT": ["kcore", "bfs", "coloring"],
    "GBBS": ["kcore", "bfs", "coloring"],
    "PASGAL": ["kcore", "bfs"],
}
SYSTEM_RENDER_ORDER = ["GBBS", "PASGAL", "ToT"]
SYSTEM_STYLES = {
    "ToT": {"marker": "o", "color": "#d62728"},
    "GBBS": {"marker": "s", "color": "#6baed6"},
    "PASGAL": {"marker": "^", "color": "#b276b2"},
}
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


def collect_all_data(base_dir: Path) -> Dict[str, Dict[str, Dict[str, List[float]]]]:
    all_data: Dict[str, Dict[str, Dict[str, List[float]]]] = {}
    data_dir = base_dir / "speedup"
    if not data_dir.is_dir():
        data_dir = base_dir
    for system, tasks in SYSTEM_TASKS.items():
        system_data: Dict[str, Dict[str, List[float]]] = {}
        for task in tasks:
            system_data[task] = read_scale_file(data_dir / f"{system}_{task}.txt")
        all_data[system] = system_data
    return all_data


def get_graph_order(all_data: Dict[str, Dict[str, Dict[str, List[float]]]]) -> List[str]:
    for system in SYSTEM_TASKS:
        for task in TASKS:
            task_data = all_data.get(system, {}).get(task)
            if task_data:
                return list(task_data.keys())
    return []


def lookup_values(
    all_data: Dict[str, Dict[str, Dict[str, List[float]]]],
    system: str,
    task: str,
    graph: str,
) -> Optional[List[float]]:
    return all_data.get(system, {}).get(task, {}).get(graph)


def draw_speedup_grid(
    all_data: Dict[str, Dict[str, Dict[str, List[float]]]],
    output_path: Path,
) -> None:
    graphs = get_graph_order(all_data)[:5]
    num_rows = len(TASKS)
    num_cols = len(graphs)
    fig, axes = plt.subplots(num_rows, num_cols, sharex=True, sharey=True, figsize=(11.5, 7.2))

    legend_handles = {}
    x_ticks = [1, 2, 4, 12, 48, 192]
    y_ticks = [1, 2, 4, 8, 16, 32, 64, 128]

    for row_idx, task in enumerate(TASKS):
        for col_idx, graph in enumerate(graphs):
            ax = axes[row_idx][col_idx]
            for system in SYSTEM_RENDER_ORDER:
                values = lookup_values(all_data, system, task, graph)
                if values is None:
                    continue

                style = SYSTEM_STYLES[system]
                (line,) = ax.plot(
                    CORES,
                    values,
                    label=system,
                    marker=style["marker"],
                    color=style["color"],
                    linewidth=1.8,
                    markersize=5,
                )
                legend_handles.setdefault(system, line)

            ax.set_xscale("log", base=2)
            ax.set_yscale("log", base=2)
            ax.set_xticks(x_ticks)
            ax.set_yticks(y_ticks)
            ax.grid(True, which="both", linestyle="--", linewidth=0.5, alpha=0.35)

            if row_idx == 0:
                ax.set_title(display_graph_name(graph), fontsize=16)
            if col_idx == 0:
                ax.set_ylabel(task, fontsize=13, fontstyle="italic")
            if row_idx == num_rows - 1:
                ax.set_xticklabels([1, 2, 4, 12, 48, "96h"], fontsize=11, rotation=0)
            else:
                ax.tick_params(labelbottom=False)

            if col_idx == 0:
                ax.set_yticklabels(["1", "2", "4", "8", "16", "32", "64", "128"], fontsize=11)
            else:
                ax.tick_params(labelleft=False)

    fig.legend(
        [legend_handles[system] for system in SYSTEM_RENDER_ORDER if system in legend_handles],
        [system for system in SYSTEM_RENDER_ORDER if system in legend_handles],
        loc="upper right",
        bbox_to_anchor=(0.985, 0.13),
        ncol=3,
        fontsize=13,
        frameon=False,
        handletextpad=0.4,
        columnspacing=1.0,
    )

    fig.tight_layout(rect=(0.04, 0.11, 1.0, 0.95))
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
        default=OUT_DIR,
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

    all_data = collect_all_data(base_dir)
    output_path = output_dir / f"speedup.{args.output_format}"
    draw_speedup_grid(all_data, output_path)
    if args.output_format == "pdf":
        (output_dir / "speedup.tex").write_text(SPEEDUP_TEX, encoding="utf-8")
    print(f"Wrote {output_path}")


if __name__ == "__main__":
    main()

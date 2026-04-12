#!/usr/bin/env python3

import csv
import math
import sys
import subprocess
from pathlib import Path

import cairo


ROOT = Path(__file__).resolve().parent
INPUTS = {
    "toggle": ROOT / "toggle" / "eu-2015-host_sym.tsv",
    "Hashbag": ROOT / "Hashbag" / "eu-2015-host_sym.tsv",
    "GBBS": ROOT / "GBBS" / "eu-2015-host_sym.tsv",
}
RUN_DIRS = {
    "toggle": ROOT / "toggle",
    "Hashbag": ROOT / "Hashbag",
    "GBBS": ROOT / "GBBS",
}
ALGORITHMS = ["BFS", "KCore", "Coloring"]


def load_rows(path):
    with path.open(newline="") as f:
        return list(csv.DictReader(f, delimiter="\t"))


def organize(rows):
    by_alg = {}
    for row in rows:
        graph = row["graph"]
        alg = row["algorithm"]
        threads = int(row["threads"])
        avg_sec = float(row["avg_sec"])
        by_alg.setdefault(alg, {"graph": graph, "times": {}})
        by_alg[alg]["times"][threads] = avg_sec
    return by_alg


def speedups(times, base):
    xs = sorted(times)
    ys = [base / times[x] for x in xs]
    return xs, ys


def draw_text(ctx, x, y, text, size=18, color=(0, 0, 0), centered=False):
    ctx.select_font_face("Sans", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(size)
    ctx.set_source_rgb(*color)
    xb, yb, width, height, xa, ya = ctx.text_extents(text)
    if centered:
        x -= width / 2 + xb
        y -= height / 2 + yb
    ctx.move_to(x, y)
    ctx.show_text(text)


def draw_chart(path, graph, alg, series):
    left, right, top, bottom = 110, 60, 80, 100
    tick_len = 8
    x_values = sorted(series["GBBS"][0])
    x_min = min(x_values)
    x_max = max(x_values)
    log_x_min = math.log2(x_min)
    log_x_max = math.log2(x_max)
    y_max = max(max(series[name][1]) for name in series)
    y_min = min(min(series[name][1]) for name in series)
    y_min = min(y_min, 1.0)
    y_max = max(1.0, y_max * 1.1)
    log_y_min = math.log2(y_min)
    log_y_max = math.log2(y_max)

    width = 1100
    plot_w = width - left - right
    x_range = max(log_x_max - log_x_min, 1.0)
    y_range = max(log_y_max - log_y_min, 1.0)
    unit_len = plot_w / x_range
    plot_h = unit_len * y_range
    height = int(math.ceil(top + plot_h + bottom))

    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    ctx = cairo.Context(surface)

    ctx.set_source_rgb(1, 1, 1)
    ctx.paint()

    def x_pos(x):
        if log_x_max == log_x_min:
            return left + plot_w / 2
        return left + (math.log2(x) - log_x_min) * plot_w / (log_x_max - log_x_min)

    def y_pos(y):
        if log_y_max == log_y_min:
            return top + plot_h / 2
        return top + plot_h - (math.log2(y) - log_y_min) * plot_h / (log_y_max - log_y_min)

    y_ticks = [0.25, 0.5, 1, 2, 4, 8, 16, 32]
    y_ticks = [y for y in y_ticks if y_min <= y <= y_max]
    for y in y_ticks:
        py = y_pos(y)
        ctx.set_source_rgb(0.1, 0.1, 0.1)
        ctx.set_line_width(2)
        ctx.move_to(left - tick_len, py)
        ctx.line_to(left, py)
        ctx.stroke()
        draw_text(ctx, left - 15, py + 5, f"{y:g}", size=15, color=(0.35, 0.35, 0.35), centered=False)

    for x in x_values:
        px = x_pos(x)
        ctx.set_source_rgb(0.1, 0.1, 0.1)
        ctx.set_line_width(2)
        ctx.move_to(px, top + plot_h)
        ctx.line_to(px, top + plot_h + tick_len)
        ctx.stroke()
        draw_text(ctx, px, top + plot_h + 30, str(x), size=15, color=(0.35, 0.35, 0.35), centered=True)

    ctx.set_source_rgb(0.1, 0.1, 0.1)
    ctx.set_line_width(2.5)
    ctx.move_to(left, top)
    ctx.line_to(left, top + plot_h)
    ctx.line_to(left + plot_w, top + plot_h)
    ctx.stroke()

    draw_text(ctx, width / 2, 38, f"{alg} Speedup on {graph}", size=28, centered=True)
    draw_text(ctx, width / 2, height - 30, "Cores", size=20, centered=True)

    ctx.save()
    ctx.translate(30, height / 2)
    ctx.rotate(-math.pi / 2)
    draw_text(ctx, 0, 0, "Speedup", size=20, centered=True)
    ctx.restore()

    styles = {
        "toggle": (0.15, 0.45, 0.8),
        "Hashbag": (0.85, 0.35, 0.1),
        "GBBS": (0.1, 0.65, 0.3),
    }

    for name, (xs, ys) in series.items():
        ctx.set_source_rgb(*styles[name])
        ctx.set_line_width(3)
        for i, x in enumerate(xs):
            px = x_pos(x)
            py = y_pos(ys[i])
            if i == 0:
                ctx.move_to(px, py)
            else:
                ctx.line_to(px, py)
        ctx.stroke()

        for i, x in enumerate(xs):
            px = x_pos(x)
            py = y_pos(ys[i])
            ctx.arc(px, py, 5.5, 0, 2 * math.pi)
            ctx.fill()

    legend_x = left + 20
    legend_y = top + 30
    for i, name in enumerate(["toggle", "Hashbag", "GBBS"]):
        y = legend_y + i * 32
        ctx.set_source_rgb(*styles[name])
        ctx.set_line_width(3)
        ctx.move_to(legend_x, y)
        ctx.line_to(legend_x + 34, y)
        ctx.stroke()
        ctx.arc(legend_x + 17, y, 5.5, 0, 2 * math.pi)
        ctx.fill()
        draw_text(ctx, legend_x + 48, y + 6, name, size=17)

    surface.write_to_png(str(path))


def main():
    rerun = True
    if len(sys.argv) >= 2:
        if sys.argv[1] not in {"0", "1"}:
            raise SystemExit("Usage: python3 plot_speedup.py [0|1]")
        rerun = (sys.argv[1] == "1")

    if rerun:
        for name, run_dir in RUN_DIRS.items():
            print(f"Running {name}/test.sh")
            subprocess.run(["./test.sh"], cwd=run_dir, check=True)

    datasets = {name: organize(load_rows(path)) for name, path in INPUTS.items()}

    for alg in ALGORITHMS:
        graph = datasets["toggle"][alg]["graph"]
        base = max(alg_data[alg]["times"][1] for alg_data in datasets.values())
        series = {name: speedups(alg_data[alg]["times"], base) for name, alg_data in datasets.items()}
        out = ROOT / f"{alg}+{graph}1.png"
        draw_chart(out, graph, alg, series)
        print(out.name)


if __name__ == "__main__":
    main()

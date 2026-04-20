import csv
import math
from pathlib import Path


MAIN_DIR = Path(__file__).resolve().parent / "main"
CSV_PATH = MAIN_DIR / "table.csv"
OUTPUT_PATH = MAIN_DIR / "rawdata.tex"

ABBREV = {
    "twitter_sym": "TW",
    "friendster_sym": "FS",
    "com-orkut_sym": "OK",
    "soc-LiveJournal1_sym": "LJ",
    "socfb-konect_sym": "FK",
    "socfb-uci-uni_sym": "FU",
    "eu-2015-host_sym": "EH",
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
    "us_sym": "US",
    "africa_sym": "AF",
    "RoadUSA_sym": "RU",
    "Germany_sym": "DE",
    "hugebubbles-00020_sym": "BBL",
    "hugetrace-00020_sym": "TRCE",
    "clueweb_sym": "CW",
    "hyperlink2014_sym": "HL14",
}

TASKS = [
    {
        "title": "BFS and KCore",
        "cols": ["F", "G", "H", "I", "J", "N", "O", "P", "Q", "R"],
    },
    {
        "title": "BellmanFord, Coloring, and WeightedBFS",
        "cols": ["C", "D", "E", "K", "L", "M", "S", "T", "U"],
    },
]


def excel_col_to_index(col_name):
    value = 0
    for char in col_name.upper():
        value = value * 26 + (ord(char) - ord("A") + 1)
    return value - 1


def load_csv(path):
    with path.open(newline="", encoding="utf-8") as file:
        rows = list(csv.reader(file))

    return rows[0], rows[1], rows[2:]


def fill_merged_header_cells(header_row):
    filled = []
    current = ""
    for cell in header_row:
        if cell:
            current = cell
        filled.append(current)
    return filled


def latex_escape(value):
    replacements = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
    }
    return "".join(replacements.get(char, char) for char in value)


def format_cell(value):
    if value == "":
        return ""
    number = float(value)
    if number == 0:
        return "0"
    decimals = max(0, 3 - math.floor(math.log10(abs(number))) - 1)
    return f"{number:.{decimals}f}"


def build_header_rows(top_header, sub_header, column_indices):
    selected_top = [top_header[index] for index in column_indices]
    selected_sub = [sub_header[index] for index in column_indices]

    first_row_parts = [""]
    cmidrules = []
    start = 0

    while start < len(selected_top):
        end = start + 1
        while end < len(selected_top) and selected_top[end] == selected_top[start]:
            end += 1

        span = end - start
        if span == 1:
            first_row_parts.append(latex_escape(selected_top[start]))
        else:
            first_row_parts.append(
                f"\\multicolumn{{{span}}}{{c}}{{{latex_escape(selected_top[start])}}}"
            )
            cmidrules.append(f"\\cmidrule(lr){{{start + 2}-{end + 1}}}")

        start = end

    second_row = " & ".join(["Graph"] + [latex_escape(cell) for cell in selected_sub])
    return " & ".join(first_row_parts), second_row, cmidrules


def render_table(top_header, sub_header, data_rows, task):
    column_indices = [excel_col_to_index(col) for col in task["cols"]]
    first_header, second_header, cmidrules = build_header_rows(
        top_header, sub_header, column_indices
    )

    align_spec = "l" + "c" * len(column_indices)
    lines = [
        r"\begin{center}",
        r"\scriptsize",
        rf"\begin{{tabular}}{{{align_spec}}}",
        r"\toprule",
        first_header + r" \\",
    ]
    lines.extend(cmidrules)
    lines.extend([second_header + r" \\", r"\midrule"])

    for row in data_rows:
        values = [format_cell(row[index]) for index in column_indices]
        graph_name = ABBREV.get(row[1], row[1])
        lines.append(" & ".join([latex_escape(graph_name)] + values) + r" \\")

    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}",
            r"\medskip",
            rf"\captionof{{table}}{{Raw data: {task['title']}}}",
            r"\end{center}",
        ]
    )

    return "\n".join(lines)


def main():
    top_header, sub_header, data_rows = load_csv(CSV_PATH)
    top_header = fill_merged_header_cells(top_header)

    tables = [
        render_table(top_header, sub_header, data_rows, task)
        for task in TASKS
    ]
    OUTPUT_PATH.write_text(
        "\\nolinenumbers\n" + "\n\n".join(tables) + "\n\\linenumbers\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()

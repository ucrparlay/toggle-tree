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

TASKS = [
    {
        "cols": ["F", "H", "J", "S", "T", "U"],
        "caption": "TBD",
    },
    {
        "cols": ["N", "P", "R", "K", "M", "C", "E"],
        "caption": "TBD",
    },
    {
        "cols": ["G", "I", "O", "Q", "L", "M", "D", "E"],
        "caption": "TBD",
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

    group_row_parts = [r"\multicolumn{2}{c}{}"]
    cmidrules = []
    group_ranges = []
    start = 0

    while start < len(selected_top):
        end = start + 1
        while end < len(selected_top) and selected_top[end] == selected_top[start]:
            end += 1

        span = end - start
        if span == 1:
            group_row_parts.append(selected_top[start])
        else:
            group_row_parts.append(f"\\multicolumn{{{span}}}{{c}}{{{selected_top[start]}}}")
            cmidrules.append(f"\\cmidrule(lr){{{start + 3}-{end + 2}}}")
        group_ranges.append(range(start, end))
        start = end

    column_row = " & ".join(
        ["Type", "Graph"] + [latex_escape(cell) for cell in selected_sub]
    )
    return " & ".join(group_row_parts), column_row, cmidrules, group_ranges


def render_table(top_header, sub_header, data_rows, task):
    column_indices = [excel_col_to_index(col) for col in task["cols"]]
    group_header, column_header, cmidrules, group_ranges = build_header_rows(
        top_header, sub_header, column_indices
    )

    align_spec = "ll" + "c" * len(column_indices)
    lines = [
        r"\begin{center}",
        r"\scriptsize",
        rf"\begin{{tabular}}{{{align_spec}}}",
        r"\toprule",
        group_header + r" \\[2pt]",
    ]
    lines.extend(cmidrules)
    lines.append(column_header + r" \\")
    lines.append(r"\midrule")

    current_type = ""
    for row in data_rows:
        type_name = row[0].strip()
        if type_name:
            if current_type:
                lines.append(r"\midrule")
            current_type = type_name

        values = [format_cell(row[index]) for index in column_indices]
        numbers = [float(row[index]) for index in column_indices]
        for group_range in group_ranges:
            min_value = min(numbers[index] for index in group_range)
            for index in group_range:
                if numbers[index] == min_value:
                    values[index] = rf"\textbf{{{values[index]}}}"

        graph_name = ABBREV.get(row[1], row[1])
        lines.append(
            " & ".join([latex_escape(type_name), latex_escape(graph_name)] + values)
            + r" \\"
        )

    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}",
            r"\medskip",
            rf"\captionof{{table}}{{{task['caption']}}}",
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

import csv
import math
from collections import OrderedDict


csv_path = r"table.csv"
significant_digits = 3
tasks = [
    {
        "outdir": "exp1.1.tex",
        "cols": ["F", "H", "J", "S", "T", "U"],
        "caption": "",
        "label": "",
    },
    {
        "outdir": "exp1.2.tex",
        "cols": ["N", "P", "R", "K", "M", "C", "E"],
        "caption": "End-to-end Comparison",
        "label": "tab:exp1",
    },
    {
        "outdir": "exp2.tex",
        "cols": ["G", "I", "O", "Q", "L", "M", "D", "E"],
        "caption": "Comparison with Hashbag",
        "label": "tab:exp2",
    },
]


def excel_col_to_index(col_name):
    value = 0
    for char in col_name.upper():
        value = value * 26 + (ord(char) - ord("A") + 1)
    return value - 1


def geometric_mean(values):
    return math.exp(sum(math.log(value) for value in values) / len(values))


def format_number(value, digits):
    if value == 0:
        return "0"
    decimals = max(0, digits - math.floor(math.log10(abs(value))) - 1)
    return f"{value:.{decimals}f}"


def load_csv(path):
    with open(path, newline="", encoding="utf-8") as file:
        rows = list(csv.reader(file))

    top_header = rows[0]
    sub_header = rows[1]
    data_rows = rows[2:]
    return top_header, sub_header, data_rows


def fill_merged_header_cells(header_row):
    filled = []
    current = ""
    for cell in header_row:
        if cell:
            current = cell
        filled.append(current)
    return filled


def group_rows_by_type(rows):
    grouped = OrderedDict()
    current_type = None

    for row in rows:
        if row[0]:
            current_type = row[0]
            grouped[current_type] = []
        grouped[current_type].append(row)

    return grouped


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
            first_row_parts.append(selected_top[start])
        else:
            first_row_parts.append(f"\\multicolumn{{{span}}}{{c}}{{{selected_top[start]}}}")
            cmidrules.append(f"\\cmidrule(lr){{{start + 2}-{end + 1}}}")

        start = end

    second_row = " & ".join([""] + selected_sub)
    return " & ".join(first_row_parts), second_row, cmidrules


def compute_rows(grouped_rows, all_rows, column_indices):
    result = []

    for group_name, rows in grouped_rows.items():
        row_values = []
        for index in column_indices:
            values = [float(row[index]) for row in rows]
            row_values.append(values)
        result.append(
            [group_name]
            + [format_number(geometric_mean(values), significant_digits) for values in row_values]
        )

    overall = []
    for index in column_indices:
        values = [float(row[index]) for row in all_rows]
        overall.append(values)
    result.append(
        ["Geomean"]
        + [format_number(geometric_mean(values), significant_digits) for values in overall]
    )
    return result


def render_table(top_header, sub_header, grouped_rows, all_rows, task):
    column_indices = [excel_col_to_index(col) for col in task["cols"]]
    first_header, second_header, cmidrules = build_header_rows(
        top_header, sub_header, column_indices
    )
    body_rows = compute_rows(grouped_rows, all_rows, column_indices)

    align_spec = "l" + "c" * len(column_indices)
    lines = [
        r"\begin{table}[htbp]",
        r"\centering",
        r"\small",
        rf"\begin{{tabular}}{{{align_spec}}}",
        r"\toprule",
        first_header + r" \\",
    ]

    lines.extend(cmidrules)
    lines.extend(
        [
            second_header + r" \\",
            r"\midrule",
        ]
    )

    for index, row in enumerate(body_rows):
        if index == len(body_rows) - 1:
            lines.append(r"\midrule")
        lines.append(" & ".join(row) + r" \\")

    lines.extend(
        [
            r"\bottomrule",
            r"\end{tabular}",
        ]
    )

    if task.get("caption"):
        lines.append(rf"\caption{{{task['caption']}}}")
    if task.get("label"):
        lines.append(rf"\label{{{task['label']}}}")
    lines.append(r"\end{table}")

    return "\n".join(lines) + "\n"


def main():
    top_header, sub_header, data_rows = load_csv(csv_path)
    top_header = fill_merged_header_cells(top_header)
    grouped_rows = group_rows_by_type(data_rows)

    for task in tasks:
        latex = render_table(top_header, sub_header, grouped_rows, data_rows, task)
        with open(task["outdir"], "w", encoding="utf-8") as file:
            file.write(latex)


if __name__ == "__main__":
    main()

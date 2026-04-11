#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

run_exp() {
    local dir="$SCRIPT_DIR/$1"
    echo "========== $1 =========="
    cd "$dir" && bash bench.sh
    echo "CSV written to $dir/rounds.csv"
    echo ""
}

run_exp Kcore_Hashbag_round
run_exp Kcore_toggle_round
run_exp Coloring_Hashbag_round
run_exp Coloring_toggle_round

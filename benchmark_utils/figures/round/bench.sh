#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_PATH="${SCRIPT_DIR}/../../scripts/config.sh"

source "${CONFIG_PATH}"

BIN_DIR_ABS="$(realpath "${BIN_DIR}")"

targets=(
    "Kcore_ToT enwiki-2023_sym"
    "Kcore_ToT indochina_sym"
    "Kcore_Hashbag enwiki-2023_sym"
    "Kcore_Hashbag indochina_sym"
    "Coloring_ToT enwiki-2023_sym"
    "Coloring_ToT indochina_sym"
    "Coloring_Hashbag enwiki-2023_sym"
    "Coloring_Hashbag indochina_sym"
    "BFS_ToT friendster_sym"
    "BFS_ToT planet_sym"
    "BFS_Hashbag friendster_sym"
    "BFS_Hashbag planet_sym"
)

built_dirs=()

build_once() {
    local folder="$1"
    for built in "${built_dirs[@]:-}"; do
        if [[ "${built}" == "${folder}" ]]; then
            return
        fi
    done

    cd "${SCRIPT_DIR}"
    cd "${folder}"
    echo "\$ make clean  [cwd=$(pwd)]"
    make clean
    echo "\$ make  [cwd=$(pwd)]"
    make
    cd "${SCRIPT_DIR}"

    built_dirs+=("${folder}")
}

run_one() {
    local folder="$1"
    local graph="$2"
    local csv="${graph}_rounds.csv"

    cd "${SCRIPT_DIR}"
    cd "${folder}"
    rm -f "${csv}"
    echo "\$ numactl -i all ./test ${BIN_DIR_ABS}/${graph}.bin ${csv}  [cwd=$(pwd)]"
    numactl -i all ./test "${BIN_DIR_ABS}/${graph}.bin" "${csv}"
    cd "${SCRIPT_DIR}"
}

for target in "${targets[@]}"; do
    read -r folder graph <<< "${target}"
    build_once "${folder}"
done

total="${#targets[@]}"
count=0
for target in "${targets[@]}"; do
    read -r folder graph <<< "${target}"
    count=$((count + 1))
    echo "[run ${count}/${total}] ${folder} -> ${graph}"
    run_one "${folder}" "${graph}"
done

echo "Completed ${total} runs."

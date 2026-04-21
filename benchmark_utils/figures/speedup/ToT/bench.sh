#!/usr/bin/env bash

set -euo pipefail

source ../../../../benchmark_utils/scripts/config.sh

BFS_DIR=../../../../benchmarks/BFS/ToT+VGC
KCORE_DIR=../../../../benchmarks/KCore/ToT+VGC+SPL
COLORING_DIR=../../../../benchmarks/Coloring/ToT
OUT_DIR=../speedup
THREADS=(1 2 4 12 24 48 96 192)

PLOT=(
    com-orkut_sym
    soc-LiveJournal1_sym
    eu-2015-host_sym
    africa_sym
    RoadUSA_sym
)

build_targets() {
    #(
    #    cd "${BFS_DIR}"
    #    make clean
    #    make
    #)
    (
        cd "${KCORE_DIR}"
        make clean
        make
    )
    #(
    #    cd "${COLORING_DIR}"
    #    make clean
    #    make
    #)
}

extract_time() {
    awk '/Running Time:/ {print $(NF-1)}' | tail -n 1
}

run_bfs() {
    local graph="$1"
    local threads="$2"
    (
        cd "${BFS_DIR}"
        PARLAY_NUM_THREADS="${threads}" numactl -i all ./test "${BIN_DIR}${graph}.bin" "${NUM_ROUNDS}"
    ) | tee /dev/stderr | extract_time
}

run_kcore() {
    local graph="$1"
    local threads="$2"
    (
        cd "${KCORE_DIR}"
        PARLAY_NUM_THREADS="${threads}" numactl -i all ./test "${BIN_DIR}${graph}.bin" "${NUM_ROUNDS}"
    ) | tee /dev/stderr | extract_time
}

run_coloring() {
    local graph="$1"
    local threads="$2"
    (
        cd "${COLORING_DIR}"
        PARLAY_NUM_THREADS="${threads}" numactl -i all ./test "${BIN_DIR}${graph}.bin" "${NUM_ROUNDS}"
    ) | tee /dev/stderr | extract_time
}

write_speedup_row() {
    local label="$1"
    local runner="$2"
    local outfile="$3"
    local graph="$4"
    local baseline=""
    local line="${graph}"

    echo "=================================================================="
    printf "%66s\n" "Algorithm: ${label}"

    for threads in "${THREADS[@]}"; do
        local avg
        avg="$("${runner}" "${graph}" "${threads}")"
        printf 'threads=%s avg=%s sec\n' "${threads}" "${avg}"
        if [[ -z "${baseline}" ]]; then
            baseline="${avg}"
        fi
        local speedup
        speedup="$(awk -v base="${baseline}" -v avg="${avg}" 'BEGIN { printf "%.12g", base / avg }')"
        line+=" ${speedup}"
    done

    printf '%s\n' "${line}" >> "${outfile}"
}

build_targets

mkdir -p "${OUT_DIR}"
: > "${OUT_DIR}/ToT_bfs.txt"
: > "${OUT_DIR}/ToT_kcore.txt"
: > "${OUT_DIR}/ToT_coloring.txt"

for g in "${PLOT[@]}"; do
    echo "=================================================================="
    printf "%66s\n" "Graph: ${g}"
    echo "Threads: ${THREADS[*]}  Rounds: ${NUM_ROUNDS}"
    echo "Outputs: ${OUT_DIR}/ToT_bfs.txt ${OUT_DIR}/ToT_kcore.txt ${OUT_DIR}/ToT_coloring.txt"

    #write_speedup_row "BFS" run_bfs "${OUT_DIR}/ToT_bfs.txt" "${g}"
    write_speedup_row "KCore" run_kcore "${OUT_DIR}/ToT_kcore.txt" "${g}"
    #write_speedup_row "Coloring" run_coloring "${OUT_DIR}/ToT_coloring.txt" "${g}"
done

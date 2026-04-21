#!/usr/bin/env bash

set -euo pipefail

source ../../../../benchmark_utils/scripts/config.sh

BAZEL_DIR=../../../../benchmark_utils/bazel
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
    (
        cd "${BAZEL_DIR}"
        bazel build @GBBS_BFS//:BFS_main @GBBS_KCore//:KCore_main @GBBS_Coloring//:GraphColoring_main -c opt
    )
}

extract_time() {
    awk '/Running Time:/ {print $(NF-1)}' | tail -n 1
}

run_target() {
    local binary="$1"
    local graph="$2"
    local threads="$3"
    (
        cd "${BAZEL_DIR}"
        PARLAY_NUM_THREADS="${threads}" numactl -i all "${binary}" -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${graph}.bin"
    ) | tee /dev/stderr | extract_time
}

write_speedup_row() {
    local label="$1"
    local binary="$2"
    local outfile="$3"
    local graph="$4"
    local baseline=""
    local line="${graph}"

    echo "=================================================================="
    printf "%66s\n" "Algorithm: ${label}"

    for threads in "${THREADS[@]}"; do
        local avg
        avg="$(run_target "${binary}" "${graph}" "${threads}")"
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
: > "${OUT_DIR}/GBBS_bfs.txt"
: > "${OUT_DIR}/GBBS_kcore.txt"
: > "${OUT_DIR}/GBBS_coloring.txt"

for g in "${PLOT[@]}"; do
    echo "=================================================================="
    printf "%66s\n" "Graph: ${g}"
    echo "Threads: ${THREADS[*]}  Rounds: ${NUM_ROUNDS}"
    echo "Outputs: ${OUT_DIR}/GBBS_bfs.txt ${OUT_DIR}/GBBS_kcore.txt ${OUT_DIR}/GBBS_coloring.txt"

    write_speedup_row "BFS" "bazel-bin/external/GBBS_BFS/BFS_main" "${OUT_DIR}/GBBS_bfs.txt" "${g}"
    write_speedup_row "KCore" "bazel-bin/external/GBBS_KCore/KCore_main" "${OUT_DIR}/GBBS_kcore.txt" "${g}"
    write_speedup_row "Coloring" "bazel-bin/external/GBBS_Coloring/GraphColoring_main" "${OUT_DIR}/GBBS_coloring.txt" "${g}"
done

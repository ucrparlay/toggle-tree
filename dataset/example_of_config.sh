#!/usr/bin/env bash

# Your directory containing graphs
GRAPH_DIR=/data/graphs/bin

# Number of test rounds after warmup
NUM_ROUNDS=5

# Compiler and flags
CXX=g++
CXXFLAGS=(-O3 -std=c++20 -pthread -march=native -Wall)

# Commented out by default. Configure it for using parlaylib scheduler.
# PARLAY_DIR=../parlaylib/include

# Clear page cache before each testcase.
CLEAR_CACHE=true

# The single graph run by test.sh
TEST=twitter

GRAPHS=(
    friendster
    twitter
    com-orkut
    socfb-konect
    soc-LiveJournal1
    # clueweb
    sd_arc
    arabic
    uk-2002
    indochina
    planet
    europe
    asia
    africa
    RoadUSA
    Cosmo50_5
    GeoLifeNoScale_10
    hugebubbles-00020
    hugetrace-00020
    CHEM_5
)

DROP_CACHES() {
    [[ "${CLEAR_CACHE}" == "true" ]] || return 0
    sudo -n sh -c 'sync; echo 3 > /proc/sys/vm/drop_caches' 2>/dev/null || :
}

if command -v numactl >/dev/null 2>&1 && numactl -i all true >/dev/null 2>&1; then
    NUMACTL=(numactl -i all)
else
    NUMACTL=()
    echo "Warning: numactl unavailable; running without NUMA memory interleaving." >&2
fi

COMPILE() {
    local root_dir="${1}"
    local graph_symmetrized="${2:-true}"
    local graph_weight_type="${3:-void}"
    local parlay_flags=()
    if [[ -n "${PARLAY_DIR}" ]]; then
        parlay_flags=(-I"${root_dir}/${PARLAY_DIR}" -DTOGGLE_TREE_USE_PARLAY)
    fi
    rm -f test
    echo "=================================================================="
    echo "Compiling..."
    TIMEFORMAT='Compilation time: %2R sec'
    time\
        "${CXX}" "${CXXFLAGS[@]}" -I"${root_dir}/include"\
        "${parlay_flags[@]}"\
        -DGRAPH_SYMMETRIZED="${graph_symmetrized}"\
        -DGRAPH_WEIGHT_TYPE="${graph_weight_type}"\
        test.cpp -o test
}

RUN_TEST_SEQUENCE() {
    local root_dir="${1}"
    local sequence_length="${2:-1000000000}"
    COMPILE "${root_dir}" || return $?
    DROP_CACHES
    "${NUMACTL[@]}" ./test "${sequence_length}" "${NUM_ROUNDS}"
    rm -f test
}

RUN_TEST_GRAPH() {
    local root_dir="${1}"
    local graph_symmetrized="${2:-true}"
    local graph_weight_type="${3:-void}"
    local suffix="${4:-_sym.bin}"
    local source_dir="${5:-"${root_dir}/dataset/sources.csv"}"
    COMPILE "${root_dir}" "${graph_symmetrized}" "${graph_weight_type}" || return $?
    DROP_CACHES
    "${NUMACTL[@]}" ./test "${GRAPH_DIR}/${TEST}${suffix}" "${NUM_ROUNDS}" "${source_dir}"
    rm -f test
}

RUN_BENCH_GRAPH() {
    local root_dir="${1}"
    local graph_symmetrized="${2:-true}"
    local graph_weight_type="${3:-void}"
    local suffix="${4:-_sym.bin}"
    local source_dir="${5:-"${root_dir}/dataset/sources.csv"}"
    COMPILE "${root_dir}" "${graph_symmetrized}" "${graph_weight_type}" || return $?
    for g in "${GRAPHS[@]}"; do
        DROP_CACHES
        "${NUMACTL[@]}" ./test "${GRAPH_DIR}/${g}${suffix}" "${NUM_ROUNDS}" "${source_dir}"
    done
    rm -f test
}

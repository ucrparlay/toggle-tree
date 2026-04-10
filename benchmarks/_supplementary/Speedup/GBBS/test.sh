#!/usr/bin/env bash
set -e

cd ../../../../benchmark_utils/bazel
source ../scripts/config.sh

OUTFILE="../../benchmarks/_supplementary/Speedup/GBBS/speedup.tsv"
rm -f "${OUTFILE}"
#BAZEL_OUT="--output_user_root=../../benchmarks/_supplementary/Speedup/GBBS"
BAZEL_CMD="bazel --batch ${BAZEL_OUT}"
SPEEDUP_REPO="--override_repository=GBBS_Speedup=../../benchmarks/_supplementary/Speedup/GBBS"
MAX_THREADS="${PARLAY_NUM_THREADS}"

run_sweep() {
    local bin="$1"
    local p=1
    while [[ "${p}" -le "${MAX_THREADS}" ]]; do
        PARLAY_NUM_THREADS="${p}" numactl -i all "${bin}" -num_rounds "${NUM_ROUNDS}" -out "${OUTFILE}" -s -b "${TEST}.bin"
        if [[ "${p}" -eq "${MAX_THREADS}" ]]; then
            break
        fi
        if [[ "${p}" -gt $((MAX_THREADS / 2)) ]]; then
            p="${MAX_THREADS}"
        else
            p=$((p * 2))
        fi
    done
}

${BAZEL_CMD} build "${SPEEDUP_REPO}" @GBBS_Speedup//:BFS_speedup_main -c opt
run_sweep bazel-bin/external/GBBS_Speedup/BFS_speedup_main

${BAZEL_CMD} build "${SPEEDUP_REPO}" @GBBS_Speedup//:KCore_speedup_main -c opt
run_sweep bazel-bin/external/GBBS_Speedup/KCore_speedup_main

${BAZEL_CMD} build "${SPEEDUP_REPO}" @GBBS_Speedup//:GraphColoring_speedup_main -c opt
run_sweep bazel-bin/external/GBBS_Speedup/GraphColoring_speedup_main

echo "------------------------------------------------------------------"
cat "${OUTFILE}"

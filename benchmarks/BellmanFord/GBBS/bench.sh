source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
export PARLAY_NUM_THREADS=$(nproc)
for g in "${WEIGHTEDGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}${g}.adj"
done
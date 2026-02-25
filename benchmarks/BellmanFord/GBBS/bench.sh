source ../../../benchmark_utils/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
for g in "${WEIGHTEDGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -rounds 1 -s "${ADJ_DIR}${g}.adj"
done
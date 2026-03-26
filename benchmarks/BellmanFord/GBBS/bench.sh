source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @GBBS_BellmanFord//:BellmanFord_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_BellmanFord/BellmanFord_main -num_rounds "${NUM_ROUNDS}" -s "${ADJ_DIR}${g}_wghlog.adj"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_BellmanFord/BellmanFord_main -num_rounds "${NUM_ROUNDS}" -s "${ADJ_DIR}${g}_wghlog.adj"
done
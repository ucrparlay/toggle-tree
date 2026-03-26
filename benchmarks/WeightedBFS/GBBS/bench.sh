source ../../benchmark_utils/config.sh
cd ../../benchmark_utils/bazel
bazel build @GBBS_WeightedBFS//:wBFS_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_WeightedBFS/wBFS_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}_wghlog.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_WeightedBFS/wBFS_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}_wghlog.bin"
done
#for g in "${WEIGHTEDGRAPHS[@]}"; do
#    numactl -i all bazel-bin/external/GBBS_WeightedBFS/wBFS_main -s "${ADJ_DIR}${g}.adj"
#done
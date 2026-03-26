source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @GBBS_Coloring//:GraphColoring_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_Coloring/GraphColoring_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/GBBS_Coloring/GraphColoring_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}.bin"
done
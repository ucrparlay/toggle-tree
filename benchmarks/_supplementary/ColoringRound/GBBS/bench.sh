source ../../../../benchmark_utils/scripts/config.sh
cd ../../../../benchmark_utils/bazel
bazel build @gbbs_coloring_round//:GraphColoring_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_coloring_round/GraphColoring_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_coloring_round/GraphColoring_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
done
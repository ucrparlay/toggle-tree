source ../../../benchmark_utils/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bfs//:BFS_main -c opt
export PARLAY_NUM_THREADS=$(nproc)
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -s -b "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -s -b "${BIN_DIR}${g}.bin"
done
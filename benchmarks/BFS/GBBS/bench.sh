source ../../.config.sh

cd ../..
bazel build //03_BFS/GBBS:BFS_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/03_BFS/GBBS/BFS_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/03_BFS/GBBS/BFS_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
done
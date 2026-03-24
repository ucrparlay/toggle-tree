source config.sh
cd ../bazel

for g in "${BIGGRAPHS[@]}"; do
    #numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}${g}_wghlog.adj"
    numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -s -b "${BIN_DIR}${g}.bin"
    numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b "${BIN_DIR}${g}.bin"
    numactl -i all bazel-bin/external/gbbs_kcore/KCore_main -s -b "${BIN_DIR}${g}.bin"
    cd ../../benchmarks
    #cd BellmanFord/ParSet && numactl -i all ./test "${ADJ_DIR}${g}_wghlog.adj" && cd ../..
    #cd BellmanFord/Hashbag && numactl -i all ./test "${ADJ_DIR}${g}_wghlog.adj" && cd ../..
    cd BFS/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd BFS/Hashbag && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd Coloring/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd Coloring/Hashbag && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd KCore/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd KCore/Hashbag && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
    cd ../benchmark_utils/bazel
done

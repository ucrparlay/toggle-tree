source ../../../benchmark_utils/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -rounds 1 -s "${ADJ_DIR}/hawaii_sym_wgh.adj"
#numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b "${BIN_DIR}/planet_sym.bin"

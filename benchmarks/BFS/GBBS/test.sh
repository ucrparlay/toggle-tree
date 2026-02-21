source ../../../benchmark_utils/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bfs//:BFS_main -c opt
numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b ../HepPh_sym.bin
#numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b "${BIN_DIR}/planet_sym.bin"
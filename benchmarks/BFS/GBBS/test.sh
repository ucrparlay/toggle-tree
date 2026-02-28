cd ../../../benchmark_utils/bazel
bazel build @gbbs_bfs//:BFS_main -c opt
numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b ../HepPh_sym.bin
source ../config.sh
numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b "${BIN_DIR}hyperlink2012_sym.bin"
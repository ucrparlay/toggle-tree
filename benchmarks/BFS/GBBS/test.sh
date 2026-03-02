cd ../../../benchmark_utils/bazel
bazel build @gbbs_bfs//:BFS_main -c opt
export PARLAY_NUM_THREADS=$(nproc)
numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -s -b ../HepPh_sym.bin
#source ../config.sh
#numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -s -b "${BIN_DIR}planet_sym.bin"
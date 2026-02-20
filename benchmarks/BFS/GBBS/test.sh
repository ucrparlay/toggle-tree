source ../../.config.sh
cd ../../../bazel
bazel build @gbbs_bfs//benchmarks/BFS/GBBS:BFS_main -c opt
#numactl -i all bazel-bin/03_BFS/GBBS/BFS_main -s -b "${BIN_DIR}/friendster_sym.bin"
#numactl -i all bazel-bin/03_BFS/GBBS/BFS_main -s -b "${BIN_DIR}/twitter_sym.bin"
numactl -i all bazel-bin/benchmarks/BFS/GBBS/BFS_main -rounds 1 -s -b "${BIN_DIR}/twitter_sym.bin"
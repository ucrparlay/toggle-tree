cd ../../../benchmark_utils/bazel
bazel build @GBBS_BellmanFord//:BellmanFord_main -c opt
numactl -i all bazel-bin/external/GBBS_BellmanFord/BellmanFord_main -num_rounds 1 -s -b ../../include/GraphIO/example/FiveStarRedFlag_sym_wghlog.bin
#source ../scripts/config.sh
#numactl -i all bazel-bin/external/GBBS_BellmanFord/BellmanFord_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}twitter_sym_wghlog.adj"
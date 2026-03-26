cd ../../../benchmark_utils/bazel
bazel build @GBBS_Coloring//:GraphColoring_main -c opt
numactl -i all bazel-bin/external/GBBS_Coloring/GraphColoring_main -num_rounds 1 -s -b ../../include/GraphIO/example/FiveStarRedFlag_sym.bin
#source ../scripts/config.sh
#numactl -i all bazel-bin/external/GBBS_Coloring/GraphColoring_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}planet_sym.bin"

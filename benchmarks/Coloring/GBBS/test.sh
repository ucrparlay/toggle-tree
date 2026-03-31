cd ../../../benchmark_utils/bazel
bazel build @GBBS_Coloring//:GraphColoring_main -c opt
source ../scripts/config.sh
numactl -i all bazel-bin/external/GBBS_Coloring/GraphColoring_main -num_rounds "${NUM_ROUNDS}" -s -b "${TEST}.bin"

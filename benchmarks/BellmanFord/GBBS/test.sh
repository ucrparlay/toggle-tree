cd ../../../benchmark_utils/bazel
bazel build @GBBS_BellmanFord//:BellmanFord_main -c opt
source ../scripts/config.sh
numactl -i all bazel-bin/external/GBBS_BellmanFord/BellmanFord_main -num_rounds "${NUM_ROUNDS}" -s -b "${TEST}_wghlog.bin"
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -round 1 -s ../hawaii_sym_wgh.adj
source ../scripts/config.sh
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}twitter_sym_wgh8.adj"
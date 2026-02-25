cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -rounds 1 -s ../hawaii_sym_wgh.adj
#source ../config.sh
#numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -rounds 1 -s "${ADJ_DIR}hawaii_sym_wgh.adj"
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
export PARLAY_NUM_THREADS=$(nproc)
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s ../hawaii_sym_wgh.adj
#source ../config.sh
#numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}hawaii_sym_wgh.adj"
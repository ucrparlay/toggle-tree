cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -round 1 -s -b ../HepPh_sym_wghlog.bin
#source ../scripts/config.sh
#numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s -b "${BIN_DIR}twitter_sym_wghlog.adj"
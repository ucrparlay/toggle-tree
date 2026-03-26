cd ../../benchmark_utils/bazel
bazel build @GBBS_WeightedBFS//:wBFS_main -c opt
source ../../benchmark_utils/config.sh
numactl -i all bazel-bin/external/GBBS_WeightedBFS/wBFS_main -s "${ADJ_DIR}friendster_sym_wghlog.adj"
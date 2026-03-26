cd ../../../../benchmark_utils/bazel
bazel build @gbbs_wbfs//:wBFS_main -c opt
source ../../../../benchmark_utils/scripts/config.sh
numactl -i all bazel-bin/external/gbbs_wbfs/wBFS_main -s "${ADJ_DIR}friendster_sym_wghlog.adj"
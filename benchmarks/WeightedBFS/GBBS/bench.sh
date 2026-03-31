source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @GBBS_WeightedBFS//:wBFS_main -c opt
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all bazel-bin/external/GBBS_WeightedBFS/wBFS_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}_wghlog.bin"
    done
done

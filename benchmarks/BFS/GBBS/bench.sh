source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @GBBS_BFS//:BFS_main -c opt

for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all bazel-bin/external/GBBS_BFS/BFS_main -num_rounds "${NUM_ROUNDS}" -s -b "${BIN_DIR}${g}.bin"
    done
done

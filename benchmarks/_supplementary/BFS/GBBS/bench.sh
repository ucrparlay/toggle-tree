source ../../../../benchmark_utils/scripts/config.sh
cd ../../../../benchmark_utils/bazel
bazel build @gbbs_sparsebfs//:SparseBFS_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_sparsebfs/SparseBFS_main -s -b "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_sparsebfs/SparseBFS_main -s -b "${BIN_DIR}${g}.bin"
done
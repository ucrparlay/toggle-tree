source ../../../benchmark_utils/scripts/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all ./kcore -s -i "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all ./kcore -s -i "${BIN_DIR}${g}.bin"
done

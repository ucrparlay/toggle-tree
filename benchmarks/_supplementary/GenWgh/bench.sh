source ../../../benchmark_utils/scripts/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    ./gen "${BIN_DIR}${g}.bin" "${ADJ_DIR}" 0
done
for g in "${SPARSEGRAPHS[@]}"; do
    ./gen "${BIN_DIR}${g}.bin" "${ADJ_DIR}" 0
done
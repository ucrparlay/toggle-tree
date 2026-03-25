source ../../../benchmark_utils/scripts/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    ./test "${BIN_DIR}${g}.bin" "${BIN_DIR}" 0
done
for g in "${SPARSEGRAPHS[@]}"; do
    ./test "${BIN_DIR}${g}.bin" "${BIN_DIR}" 0
done
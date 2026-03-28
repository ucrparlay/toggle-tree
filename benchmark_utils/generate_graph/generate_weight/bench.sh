source ../../scripts/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    ./test "${BIN_DIR}${g}.bin" "${BIN_DIR}" 0 ".bin"
    #./test "${BIN_DIR}${g}.bin" "${ADJ_DIR}" 0 ".adj"
done
for g in "${SPARSEGRAPHS[@]}"; do
    ./test "${BIN_DIR}${g}.bin" "${BIN_DIR}" 0 ".bin"
    #./test "${BIN_DIR}${g}.bin" "${ADJ_DIR}" 0 ".adj"
done
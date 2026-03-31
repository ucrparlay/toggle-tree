source ../../scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        ./test "${BIN_DIR}${g}.bin" "${BIN_DIR}" 0 ".bin"
        ./test "${BIN_DIR}${g}.bin" "${ADJ_DIR}" 0 ".adj"
    done
done

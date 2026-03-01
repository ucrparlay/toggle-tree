source ../../../benchmark_utils/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all ./test "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all ./test "${BIN_DIR}${g}.bin"
done
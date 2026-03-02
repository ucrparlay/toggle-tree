source ../../../benchmark_utils/config.sh
make clean
make
export PARLAY_NUM_THREADS=$(nproc)
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all ./test "${BIN_DIR}${g}.bin"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all ./test "${BIN_DIR}${g}.bin"
done
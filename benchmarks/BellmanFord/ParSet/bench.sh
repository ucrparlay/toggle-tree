source ../../../benchmark_utils/config.sh
make clean
make
for g in "${WEIGHTEDGRAPHS[@]}"; do
    numactl -i all ./test "${ADJ_DIR}${g}.adj"
done
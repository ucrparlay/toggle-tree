source ../../../../benchmark_utils/scripts/config.sh
make clean
make bfs
#for g in "${DENSEGRAPHS[@]}"; do
#    numactl -i all ./bfs -i "${BIN_DIR}${g}.bin" -s -n "${NUM_ROUNDS}"
#done
#for g in "${SPARSEGRAPHS[@]}"; do
#    numactl -i all ./bfs -i "${BIN_DIR}${g}.bin" -s -n "${NUM_ROUNDS}"
#done
for g in "${DIRECTEDGRAPHS[@]}"; do
    numactl -i all ./bfs -i "${BIN_DIR}${g}.bin" -n "${NUM_ROUNDS}"
done
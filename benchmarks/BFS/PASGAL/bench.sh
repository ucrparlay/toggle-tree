source ../../../benchmark_utils/scripts/config.sh
make clean
make bfs
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all ./bfs -i "${BIN_DIR}${g}.bin" -n "${NUM_ROUNDS}"
    done
done
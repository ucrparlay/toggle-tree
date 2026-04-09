source ../../../../benchmark_utils/scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all ./kcore -i "${BIN_DIR}${g}.bin" -n "${NUM_ROUNDS}"
    done
done
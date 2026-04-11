source ../../../benchmark_utils/scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all ./test "${BIN_DIR}${g}.bin" "${NUM_ROUNDS}"
    done
done


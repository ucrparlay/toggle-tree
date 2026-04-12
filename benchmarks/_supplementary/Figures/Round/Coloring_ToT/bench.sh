source ../../../../../benchmark_utils/scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        rm -f "${g}_rounds.csv"
        numactl -i all ./test "${BIN_DIR}${g}.bin" "${g}_rounds.csv"
    done
done

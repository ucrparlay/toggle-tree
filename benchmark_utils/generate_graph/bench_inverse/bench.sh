source ../../scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        ./test "${BIN_DIR}${g}.bin"
    done
done
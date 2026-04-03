source ../../../../benchmark_utils/scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        numactl -i all ./sssp -i "${ADJ_DIR}${g}_wghlog.adj" -p 2000000 -w -s -v -a rho-stepping
    done
done

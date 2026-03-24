source ../../../benchmark_utils/scripts/config.sh
make clean
make
for g in "${DENSEGRAPHS[@]}"; do
    numactl -i all ./test "${ADJ_DIR}${g}_wghlog.adj"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all ./test "${ADJ_DIR}${g}_wghlog.adj"
done
#for g in "${WEIGHTEDGRAPHS[@]}"; do
#    numactl -i all ./test "${ADJ_DIR}${g}.adj"
#done
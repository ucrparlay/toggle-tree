make clean
make
numactl -i all ./test ../../../benchmark_utils/hawaii_sym_wgh.adj
#source ../../../benchmark_utils/config.sh
#numactl -i all ./test "${ADJ_DIR}hawaii_sym_wgh.adj"
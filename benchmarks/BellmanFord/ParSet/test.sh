source ../../../benchmark_utils/config.sh
make clean
make
numactl -i all ./test "${ADJ_DIR}/hawaii_sym_wgh.adj"

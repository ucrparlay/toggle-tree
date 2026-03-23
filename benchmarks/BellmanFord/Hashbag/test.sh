make clean
make
numactl -i all ./test ../../../benchmark_utils/hawaii_sym_wgh.adj
source ../../../benchmark_utils/scripts/config.sh
numactl -i all ./test "${ADJ_DIR}twitter_sym_wgh8.adj"
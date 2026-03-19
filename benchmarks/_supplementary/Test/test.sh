source ../../../benchmark_utils/scripts/config.sh
make clean
make
taskset -c 0 numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 1000000
taskset -c 0 numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 100000
taskset -c 0 numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 10000
taskset -c 0 numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 1000
source ../../../benchmark_utils/scripts/config.sh
make clean
make
numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 1000000
numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 100000
numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 10000
numactl -i all ./test "${BIN_DIR}twitter_sym.bin" 1000
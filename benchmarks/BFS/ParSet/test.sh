source ../../../benchmark_utils/config.sh
make clean
make
#numactl -i all ./test "${BIN_DIR}/eu-2015-host_sym.bin"
#numactl -i all ./test "${BIN_DIR}/friendster_sym.bin"
#numactl -i all ./test "${BIN_DIR}/skitter_sym.bin"
numactl -i all ./test "${BIN_DIR}/planet_sym.bin"

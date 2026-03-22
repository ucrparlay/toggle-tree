make clean
make
numactl -i all ./test ../../../benchmark_utils/HepPh_sym.bin
source ../../../benchmark_utils/scripts/config.sh
numactl -i all ./test "${BIN_DIR}europe_sym.bin"
source ../../../benchmark_utils/config.sh
make clean
make
numactl -i all ./test ../../../benchmark_utils/HepPh_sym.bin
numactl -i all ./test "${BIN_DIR}/indochina_sym.bin"
numactl -i all ./test "${BIN_DIR}/planet_sym.bin"

make clean
make
numactl -i all ./test ../../../benchmark_utils/HepPh_sym.bin
source ../../../benchmark_utils/scripts/config.sh
numactl -i all ./test "${BIN_DIR}uk-2002_sym.bin"
numactl -i all ./test "${BIN_DIR}arabic_sym.bin"
numactl -i all ./test "${BIN_DIR}eu-2015-host_sym.bin"
numactl -i all ./test "${BIN_DIR}indochina_sym.bin"
numactl -i all ./test "${BIN_DIR}twitter_sym.bin"
numactl -i all ./test "${BIN_DIR}sd_arc_sym.bin"
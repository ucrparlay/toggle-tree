source ../../../benchmark_utils/scripts/config.sh
make clean
make
./test "${BIN_DIR}twitter_sym.bin" "${ADJ_DIR}" 8
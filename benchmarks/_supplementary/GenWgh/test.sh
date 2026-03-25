source ../../../benchmark_utils/scripts/config.sh
make clean
make
./test "${BIN_DIR}twitter_sym.bin" "${BIN_DIR}" 0
make clean
make
#numactl -i all ./test ../../../include/GraphIO/example/FiveStarRedFlag_sym.bin 1 
source ../../../benchmark_utils/scripts/config.sh
numactl -i all ./test "${BIN_DIR}sd_arc_sym.bin" "${NUM_ROUNDS}"
numactl -i all ./test "${BIN_DIR}twitter_sym.bin" "${NUM_ROUNDS}"

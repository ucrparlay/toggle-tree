make clean
make
numactl -i all ./test ../../../include/GraphIO/example/FiveStarRedFlag_sym.bin 1 
source ../../../benchmark_utils/scripts/config.sh
numactl -i all ./test /data/graphs/bin/twitter_sym_wghlog.bin "${NUM_ROUNDS}"
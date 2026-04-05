make clean
make
source ../../../../benchmark_utils/scripts/config.sh
numactl -i all ./test "${TEST}_wghlog.bin" "${NUM_ROUNDS}"
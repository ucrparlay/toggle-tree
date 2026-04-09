make clean
make
source ../../../../benchmark_utils/scripts/config.sh
numactl -i all ./bfs -i "${TEST}.bin" -s -n "${NUM_ROUNDS}"
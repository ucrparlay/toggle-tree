source ../../../../benchmark_utils/scripts/config.sh
make clean
make bfs
numactl -i all ./bfs -i "${BIN_DIR}/friendster_sym.bin" -s -n 1
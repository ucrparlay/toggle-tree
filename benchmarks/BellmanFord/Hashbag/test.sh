make clean
make
numactl -i all ./test ../../../benchmark_utils/HepPh_sym_wghlog.bin
#source ../../../benchmark_utils/scripts/config.sh
#numactl -i all ./test "${BIN_DIR}twitter_sym_wghlog.bin"
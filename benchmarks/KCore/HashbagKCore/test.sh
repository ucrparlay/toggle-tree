make clean
make
numactl -i all ./kcore -s -i ../../../benchmark_utils/HepPh_sym.bin

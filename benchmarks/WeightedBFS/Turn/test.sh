make clean
make
#numactl -i all ./test ../../benchmark_utils/hawaii_sym_wgh.adj
#source ../../benchmark_utils/scripts/config.sh
#numactl -i all ./test "${ADJ_DIR}hawaii_sym_wgh.adj"
#numactl -i all ./test /data/graphs/pbbs/CHEM_10_sym_wgh.adj
numactl -i all ./test /data/graphs/bin/twitter_sym_wghlog.bin
numactl -i all ./test /data/graphs/pbbs/twitter_sym_wghlog.adj
#numactl -i all ./test /data/graphs/pbbs/planet_sym_wghlog.adj
#numactl -i all ./test /data/graphs/pbbs/hawaii_sym_wgh.adj
#numactl -i all ./test /data/graphs/pbbs/alaska_sym_wgh.adj
#numactl -i all ./test /data/graphs/pbbs/HT_10_sym_wgh.adj
#numactl -i all ./test /data/graphs/pbbs/Household_10_sym_wgh.adj
#numactl -i all ./test /data/graphs/pbbs/CHEM_10_sym_wgh.adj

#numactl -i all ./test /home/csgrads/xjian140/data/graphs/pbbs/RoadUSA_sym_wgh.adj
#numactl -i all ./test /home/csgrads/xjian140/data/graphs/pbbs/Household_10_sym_wgh.adj
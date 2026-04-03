make clean
make
#./sssp -i ../../benchmark_utils/hawaii_sym_wgh.adj -p 2000000 -w -s -v -a rho-stepping
#./sssp -i /home/csgrads/xjian140/data/graphs/pbbs/RoadUSA_sym_wgh.adj -p 2000000 -w -s -v -a rho-stepping
#./sssp -i /home/csgrads/xjian140/data/graphs/pbbs/twitter_wgh18.adj -p 2000000 -w -s -v -a rho-stepping
./sssp -i /data/graphs/pbbs/planet_sym_wghlog.adj -p 2000000 -w -s -v -a rho-stepping

#make clean
#make
source ../../../../benchmark_utils/scripts/config.sh
./sssp -i "${TEST}_wghlog.bin" -p 2000000 -w -s -v -a rho-stepping
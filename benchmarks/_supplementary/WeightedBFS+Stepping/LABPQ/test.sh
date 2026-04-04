#make clean
#make
source ../../../../benchmark_utils/scripts/config.sh
TEST2="${TEST/#$BIN_DIR/$ADJ_DIR}"
echo "$TEST2"
./sssp -i "${TEST2}_wghlog.adj" -p 2000000 -w -s -v -a rho-stepping
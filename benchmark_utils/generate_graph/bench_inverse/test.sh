make clean
make
source ../../scripts/config.sh
./test "${BIN_DIR}twitter.bin"
./test "${BIN_DIR}planet.bin"
make clean
make
#./test ../../../include/GraphIO/example/FiveStarRedFlag_sym.bin ../../../include/GraphIO/example/ 0 ".bin"
#./test ../../../include/GraphIO/example/FiveStarRedFlag_sym.bin ../../../include/GraphIO/example/ 0 ".adj"
source ../../scripts/config.sh
#./test "${BIN_DIR}hyperlink2014_sym.bin" "${BIN_DIR}" 0 ".bin"
./test "${BIN_DIR}clueweb_sym.bin" "${BIN_DIR}" 0 ".bin"
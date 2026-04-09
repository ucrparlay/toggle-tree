#!/usr/bin/env bash

make clean
make
source ../../../../benchmark_utils/scripts/config.sh

OUTFILE="speedup.tsv"
numactl -i all ./test "${TEST}.bin" "${NUM_ROUNDS}" "${OUTFILE}"
echo "------------------------------------------------------------------"
cat "${OUTFILE}"

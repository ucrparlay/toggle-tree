#!/usr/bin/env bash

make clean
make
source ../../../../benchmark_utils/scripts/config.sh

PLOT=(
    com-orkut_sym
    soc-LiveJournal1_sym
    eu-2015-host_sym
    africa_sym
    RoadUSA_sym
)

for g in "${PLOT[@]}"; do
    numactl -i all ./test "${BIN_DIR}${g}.bin" "${NUM_ROUNDS}"
done


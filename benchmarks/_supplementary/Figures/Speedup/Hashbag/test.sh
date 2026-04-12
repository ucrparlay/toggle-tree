#!/usr/bin/env bash

make clean
make
source ../../../../../benchmark_utils/scripts/config.sh

numactl -i all ./test "${TEST}.bin" "${NUM_ROUNDS}"
echo "------------------------------------------------------------------"
for f in bfs.txt kcore.txt coloring.txt; do
    if [[ -f "$f" ]]; then
        echo "$f"
        tail -n 1 "$f"
    fi
done

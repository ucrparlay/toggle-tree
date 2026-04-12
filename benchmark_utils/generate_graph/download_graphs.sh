#!/usr/bin/env bash

source ../scripts/config.sh
make clean
make
for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        f="${BIN_DIR}${g}.bin"
        [ -f "$f" ] && continue
        wget -O "$f" "https://pasgal-bs.cs.ucr.edu/bin/${g}.bin" || rm -f "$f"
    done
done

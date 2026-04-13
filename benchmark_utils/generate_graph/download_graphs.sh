#!/usr/bin/env bash

source ../scripts/config.sh

for type in "${TYPES[@]}"; do
    eval "graphs=(\"\${${type}[@]}\")"
    for g in "${graphs[@]}"; do
        f="${BIN_DIR}${g}.bin"
        [ -f "$f" ] && continue
        [[ "$g" == *Cosmo* || "$g" == *GeoLifeNoScale* || "$g" == *CHEM* ]] && d=knn || d=bin
        wget -O "$f" "https://pasgal-bs.cs.ucr.edu/${d}/${g}.bin" || rm -f "$f"
    done
done
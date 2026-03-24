#!/usr/bin/env bash
source config.sh
for g in "${DENSEGRAPHS[@]}"; do
    f="${BIN_DIR}${g}.bin"
    [ -f "$f" ] && continue
    wget -O "$f" "https://pasgal-bs.cs.ucr.edu/bin/${g}.bin" || rm -f "$f"
done
for g in "${SPARSEGRAPHS[@]}"; do
    f="${BIN_DIR}${g}.bin"
    [ -f "$f" ] && continue
    wget -O "$f" "https://pasgal-bs.cs.ucr.edu/bin/${g}.bin" || rm -f "$f"
done
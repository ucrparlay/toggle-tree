# To run benchmarks:
# Copy this file, rename it as "config.sh"
# change BIN_DIR & ADJ_DIR to your directory containing graphs,
# and configure PARLAY_NUM_THREADS as you wish.

export PARLAY_NUM_THREADS=$(nproc)
NUM_ROUNDS=5
BIN_DIR=/data/graphs/bin/
ADJ_DIR=/data/graphs/pbbs/

SOCIALGRAPHS=(
    twitter
    friendster
    com-orkut
    soc-LiveJournal1
    socfb-konect
    socfb-uci-uni
)

WEBGRAPHS=(
    eu-2015-host
    sd_arc
    enwiki-2023
    arabic
    uk-2002
    indochina
)

BIGWEBGRAPHS=(
    clueweb
    hyperlink2014
    hyperlink2012
)

ROADGRAPHS=(
    planet
    europe
    asia
    north-america
    us
    africa
    RoadUSA
    Germany
)

SYNTHETICGRAPHS=(
    hugebubbles-00020
    hugetrace-00020
)

KNNGRAPHS=(
    GeoLifeNoScale_10
    CHEM_5
    Cosmo50_5
)
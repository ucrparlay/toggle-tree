# To run benchmarks:
# Copy this file, rename it as "config.sh"
# change BIN_DIR & ADJ_DIR to your directory containing graphs,
# and configure PARLAY_NUM_THREADS as you wish.

export PARLAY_NUM_THREADS=$(nproc)
NUM_ROUNDS=5
BIN_DIR=/data/graphs/bin/
ADJ_DIR=/data/graphs/pbbs/
#TEST=../../include/GraphIO/example/FiveStarRedFlag_sym
#TEST="${BIN_DIR}clueweb_sym"

TYPES=(
    SOCIAL
    WEB
    ROAD
    SYNTHETIC
)
SOCIAL=(
    twitter_sym
    friendster_sym
    com-orkut_sym
    soc-LiveJournal1_sym
    socfb-konect_sym
    socfb-uci-uni_sym
)
WEB=(
    eu-2015-host_sym
    sd_arc_sym
    enwiki-2023_sym
    arabic_sym
    uk-2002_sym
    indochina_sym
)
ROAD=(
    planet_sym
    europe_sym
    asia_sym
    north-america_sym
    us_sym
    africa_sym
    RoadUSA_sym
    Germany_sym
)
SYNTHETIC=(
    GeoLifeNoScale_10_sym
    CHEM_5_sym
    Cosmo50_5_sym
    hugebubbles-00020_sym
    hugetrace-00020_sym
)

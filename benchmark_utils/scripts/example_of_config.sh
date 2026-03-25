# To run benchmarks: 
# Copy this file, rename it as "config.sh"
# change BIN_DIR & ADJ_DIR to your diectory containing graphs, 
# and configure PARLAY_NUM_THREADS as you wish.
export PARLAY_NUM_THREADS=$(nproc)
BIN_DIR=/data/graphs/bin/
ADJ_DIR=/data/graphs/pbbs/
DENSEGRAPHS=(
    twitter_sym
    com-orkut_sym
    friendster_sym
    soc-LiveJournal1_sym
    eu-2015-host_sym
    sd_arc_sym
    enwiki-2023_sym
    arabic_sym
    uk-2002_sym
    indochina_sym
    socfb-konect_sym
    socfb-uci-uni_sym
)
SPARSEGRAPHS=(
    planet_sym
    europe_sym
    asia_sym
    north-america_sym
    us_sym
    africa_sym
    hugebubbles-00020_sym
    RoadUSA_sym
    hugetrace-00020_sym
    Germany_sym
)
BIGGRAPHS=(
    clueweb_sym
    hyperlink2014_sym
    hyperlink2012_sym
)
DIRECTEDGRAPHS=(
    twitter
    soc-LiveJournal1
    eu-2015-host
    sd_arc
    enwiki-2023
    planet
    europe
    asia
    north-america
    us
    africa
)
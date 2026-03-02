cd ../../../benchmark_utils/bazel
bazel build @gbbs_coloring//:GraphColoring_main -c opt
export PARLAY_NUM_THREADS=$(nproc)
numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b ../HepPh_sym.bin
#source ../config.sh
#numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b "${BIN_DIR}planet_sym.bin"
#numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b "${BIN_DIR}clueweb_sym.bin"
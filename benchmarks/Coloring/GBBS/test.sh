cd ../../../benchmark_utils/bazel
bazel build @gbbs_coloring//:GraphColoring_main -c opt
numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b ../HepPh_sym.bin
#source ../scripts/config.sh
#numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -s -b "${BIN_DIR}planet_sym.bin"
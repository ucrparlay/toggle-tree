cd ../../../../benchmark_utils/bazel
bazel build @gbbs_coloring_round//:GraphColoring_main -c opt
numactl -i all bazel-bin/external/gbbs_coloring_round/GraphColoring_main -rounds 1 -s -b ../HepPh_sym.bin
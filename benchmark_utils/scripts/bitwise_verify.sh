# Make sure you have compiled both GBBS and ToT implementations

TEST_ALGO=BFS                    # BellmanFord BFS Coloring KCore WeightedBFS
TEST_FOLDER=ToT+VGC
TEST_GRAPH=Germany_sym           # Don't forget to add _wghlog for weighted graphs

# TEST_GBBS=bazel-bin/external/GBBS_BellmanFord/BellmanFord_main
TEST_GBBS=bazel-bin/external/GBBS_BFS/BFS_main
# TEST_GBBS=bazel-bin/external/GBBS_Coloring/GraphColoring_main
# TEST_GBBS=bazel-bin/external/GBBS_KCore/KCore_main
# TEST_GBBS=bazel-bin/external/GBBS_WeightedBFS/wBFS_main

source config.sh
TEST_DIR="$BIN_DIR"

cd ../bazel
numactl -i all $TEST_GBBS -dump "../DumpedGBBS.txt" -num_rounds 1 -s -b "$BIN_DIR$TEST_GRAPH.bin"

cd ../../benchmarks/$TEST_ALGO/$TEST_FOLDER
numactl -i all ./test "$BIN_DIR$TEST_GRAPH.bin" 1 ../../../benchmark_utils/DumpedToT.txt

cd ../../../benchmark_utils
echo "====================== bitwise verification ======================"
cmp -s DumpedGBBS.txt DumpedToT.txt && echo "Exactly Same" || cmp DumpedGBBS.txt DumpedToT.txt | head -n 1
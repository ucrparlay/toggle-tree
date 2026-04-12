# Make sure you have compiled both GBBS and Parset implementations through test_gbbs.sh and test_parset.sh

TEST_ALGO=BFS               # BFS Coloring KCore BellmanFord
TEST_GRAPH=twitter_sym
TEST_TYPE=bin               # bin adj
TEST_GBBS=bazel-bin/external/gbbs_bfs/BFS_main
# TEST_GBBS=bazel-bin/external/gbbs_coloring/GraphColoring_main
# TEST_GBBS=bazel-bin/external/gbbs_kcore/KCore_main
# TEST_GBBS=bazel-bin/external/gbbs_bellman_ford/BellmanFord_main

source config.sh
if [[ "$TEST_TYPE" == "bin" ]]; then
  TEST_DIR="$BIN_DIR"
else
  TEST_DIR="$ADJ_DIR"
fi

cd ../bazel
if [[ "$TEST_TYPE" == "bin" ]]; then
  numactl -i all $TEST_GBBS -dump "../DumpedGBBS.txt" -s -b "$TEST_DIR$TEST_GRAPH.$TEST_TYPE"
else
  numactl -i all $TEST_GBBS -dump "../DumpedGBBS.txt" -s "$TEST_DIR$TEST_GRAPH.$TEST_TYPE"
fi

cd ../../benchmarks/$TEST_ALGO/ToT
numactl -i all ./test "$TEST_DIR$TEST_GRAPH.$TEST_TYPE" ../../../benchmark_utils/DumpedParSet.txt

cd ../../../benchmark_utils
echo "================ bitwise verification ================"
cmp -s DumpedGBBS.txt DumpedParSet.txt && echo "exactly same" || cmp DumpedGBBS.txt DumpedParSet.txt | head -n 1
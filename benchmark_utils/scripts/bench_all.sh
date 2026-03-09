cd ../../benchmarks

rm BFS/*.csv
rm Coloring/*.csv
rm KCore/*.csv
rm BellmanFord/*.csv

cd BFS/ParSet && ./bench.sh && cd ../..
cd BFS/Hashbag && ./bench.sh && cd ../..
cd Coloring/ParSet && ./bench.sh && cd ../..
cd Coloring/Hashbag && ./bench.sh && cd ../..
cd KCore/ParSetKCore && ./bench.sh && cd ../..

cd BFS/GBBS && ./bench.sh && cd ../..
cd Coloring/GBBS && ./bench.sh && cd ../..
cd KCore/HashbagKCore && ./bench.sh && cd ../..

cd BellmanFord/ParSet && ./bench.sh && cd ../..
cd BellmanFord/GBBS && ./bench.sh && cd ../..

#source ../benchmark_utils/config.sh
#for g in "${BIGGRAPHS[@]}"; do
#    cd BFS/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
#    cd Coloring/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
#    cd KCore/ParSet && numactl -i all ./test "${BIN_DIR}${g}.bin" && cd ../..
#    cd ../benchmark_utils/bazel
#    numactl -i all bazel-bin/external/gbbs_bfs/BFS_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
#    numactl -i all bazel-bin/external/gbbs_coloring/GraphColoring_main -rounds 1 -s -b "${BIN_DIR}${g}.bin"
#    numactl -i all bazel-bin/external/gbbs_kcore/KCore_main -rounds 1 -s -b "${BIN_DIR}${g}.bin" # Segmentation fault on hyperlink2014
#    cd ../../benchmarks
#done

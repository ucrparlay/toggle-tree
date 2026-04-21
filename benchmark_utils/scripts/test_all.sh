cd ../../benchmarks

cd BellmanFord/ToT && bash ./test.sh && cd ../..
cd BellmanFord/GBBS && bash ./test.sh && cd ../..
cd BellmanFord/Hashbag && bash ./test.sh && cd ../..

cd BFS/ToT+VGC && bash ./test.sh && cd ../..
cd BFS/ToT && bash ./test.sh && cd ../..
cd BFS/PASGAL && bash ./test.sh && cd ../..
cd BFS/GBBS && bash ./test.sh && cd ../..
cd BFS/Hashbag && bash ./test.sh && cd ../..

cd Coloring/ToT && bash ./test.sh && cd ../..
cd Coloring/GBBS && bash ./test.sh && cd ../..
cd Coloring/Hashbag && bash ./test.sh && cd ../..

cd KCore/ToT+VGC+SPL && bash ./test.sh && cd ../..
cd KCore/ToT && bash ./test.sh && cd ../..
cd KCore/PASGAL && bash ./test.sh && cd ../..
cd KCore/GBBS && bash ./test.sh && cd ../..
cd KCore/Hashbag && bash ./test.sh && cd ../..

cd WeightedBFS/ToT_IM && bash ./test.sh && cd ../..
cd WeightedBFS/ToT_IS && bash ./test.sh && cd ../..
cd WeightedBFS/GBBS && bash ./test.sh && cd ../..

cd ../../benchmarks

rm -f BellmanFord/*.csv
rm -f BFS/*.csv
rm -f Coloring/*.csv
rm -f KCore/*.csv
rm -f WeightedBFS/*.csv

cd BellmanFord/ToT && bash ./bench.sh && cd ../..
cd BellmanFord/GBBS && bash ./bench.sh && cd ../..
cd BellmanFord/Hashbag && bash ./bench.sh && cd ../..

cd BFS/ToT+VGC && bash ./bench.sh && cd ../..
cd BFS/ToT && bash ./bench.sh && cd ../..
cd BFS/PASGAL && bash ./bench.sh && cd ../..
cd BFS/GBBS && bash ./bench.sh && cd ../..
cd BFS/Hashbag && bash ./bench.sh && cd ../..

cd Coloring/ToT && bash ./bench.sh && cd ../..
cd Coloring/GBBS && bash ./bench.sh && cd ../..
cd Coloring/Hashbag && bash ./bench.sh && cd ../..

cd KCore/ToT+VGC+SPL && bash ./bench.sh && cd ../..
cd KCore/ToT && bash ./bench.sh && cd ../..
cd KCore/PASGAL && bash ./bench.sh && cd ../..
cd KCore/GBBS && bash ./bench.sh && cd ../..
cd KCore/Hashbag && bash ./bench.sh && cd ../..

cd WeightedBFS/ToT_IM && bash ./bench.sh && cd ../..
cd WeightedBFS/ToT_IS && bash ./bench.sh && cd ../..
cd WeightedBFS/GBBS && bash ./bench.sh && cd ../..

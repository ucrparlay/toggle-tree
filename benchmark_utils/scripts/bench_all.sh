cd ../../benchmarks

rm -f BFS/*.csv
rm -f Coloring/*.csv
rm -f KCore/*.csv
rm -f BellmanFord/*.csv
rm -f WeightedBFS/*.csv

cd BFS/ToT && bash ./bench.sh && cd ../..
cd BFS/Hashbag && bash ./bench.sh && cd ../..
cd BFS/GBBS && bash ./bench.sh && cd ../..

cd Coloring/ToT && bash ./bench.sh && cd ../..
cd Coloring/Hashbag && bash ./bench.sh && cd ../..
cd Coloring/GBBS && bash ./bench.sh && cd ../..

cd KCore/ToT && bash ./bench.sh && cd ../..
cd KCore/Hashbag && bash ./bench.sh && cd ../..
cd KCore/GBBS && bash ./bench.sh && cd ../..

cd BellmanFord/ToT && bash ./bench.sh && cd ../..
cd BellmanFord/Hashbag && bash ./bench.sh && cd ../..
cd BellmanFord/GBBS && bash ./bench.sh && cd ../..

cd WeightedBFS/ToT-IndexSet && bash ./bench.sh && cd ../..
cd WeightedBFS/ToT-IndexMap && bash ./bench.sh && cd ../..
cd WeightedBFS/GBBS && bash ./bench.sh && cd ../..
rm BFS/*.csv
rm Coloring/*.csv
rm KCore/*.csv
rm MIS/*.csv
rm BellmanFord/*.csv

cd BFS/ParSet && ./bench.sh && cd ../..
cd Coloring/ParSet && ./bench.sh && cd ../..
cd KCore/ParSet && ./bench.sh && cd ../..
cd MIS/ParSet && ./bench.sh && cd ../..
cd BellmanFord/ParSet && ./bench.sh && cd ../..

cd BFS/GBBS && ./bench.sh && cd ../..
cd Coloring/GBBS && ./bench.sh && cd ../..
cd KCore/GBBS && ./bench.sh && cd ../..
cd MIS/GBBS && ./bench.sh && cd ../..
cd BellmanFord/GBBS && ./bench.sh && cd ../..
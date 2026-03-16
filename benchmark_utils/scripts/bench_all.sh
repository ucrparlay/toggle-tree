cd ../../benchmarks

# rm -f BFS/*.csv
# rm -f Coloring/*.csv
# rm -f KCore/*.csv
# rm -f BellmanFord/*.csv

# cd BFS/ParSet && bash ./bench.sh && cd ../..
# cd BFS/GBBS && bash ./bench.sh && cd ../..

# cd Coloring/ParSet && bash ./bench.sh && cd ../..
# cd Coloring/GBBS && bash ./bench.sh && cd ../..

# cd KCore/ParSet && bash ./bench.sh && cd ../..
# cd KCore/GBBS && bash ./bench.sh && cd ../..

# cd BellmanFord/ParSet && bash ./bench.sh && cd ../..
# cd BellmanFord/GBBS && bash ./bench.sh && cd ../..

# benchmarks in _supplementary
cd _supplementary

rm -f BFS/*.csv
rm -f KCore/*.csv
rm -f ColoringRound/*.csv

cd BFS/Hashbag && bash ./bench.sh && cd ../..

cd KCore/Hashbag && bash ./bench.sh && cd ../..
cd KCore/HashbagKCore && bash ./bench.sh && cd ../..
cd KCore/HashbagSampling && bash ./bench.sh && cd ../..

cd ColoringRound/Hashbag && bash ./bench.sh && cd ../..

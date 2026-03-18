# benchmarks in _supplementary
cd ./../../benchmarks/_supplementary

rm -f BFS/*.csv
rm -f KCore/*.csv
# rm -f ColoringRound/*.csv

# cd BFS/Hashbag && bash ./bench.sh && cd ../..
# cd BFS/ParSet && bash ./bench.sh && cd ../..

# cd KCore/HashbagKCore && bash ./bench.sh && cd ../..
# cd KCore/ParSetKCore && bash ./bench.sh && cd ../..
# cd KCore/HashbagSampling && bash ./bench.sh && cd ../..

# cd KCore/Hashbag && bash ./bench.sh && cd ../..
# cd KCore/HashbagKCore && bash ./bench.sh && cd ../..
cd KCore/HashbagSampling && bash ./bench.sh && cd ../..
cd KCore/ParSetSampling && bash ./bench.sh && cd ../..

# cd ColoringRound/Hashbag && bash ./bench.sh && cd ../..
# cd ColoringRound/ParSet && bash ./bench.sh && cd ../..

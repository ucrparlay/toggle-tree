cd ../../benchmarks/_supplementary

cd BFS/Hashbag && bash ./bench.sh && cd ../..

cd Hashbag && bash ./bench.sh && cd ..

cd ParSetKCore && bash ./bench.sh && cd ..
cd HashbagKCore && bash ./bench.sh && cd ..
cd ParSetSampling && bash ./bench.sh && cd ..
cd HashbagSampling && bash ./bench.sh && cd ..

cd KCore/Pack && bash ./bench.sh && cd ../..
cd KCore/Hashbag && bash ./bench.sh && cd ../..
cd KCore/Augument && bash ./bench.sh && cd ../..
cd KCore/ParSetSampling && bash ./bench.sh && cd ../..
cd KCore/HashbagSampling && bash ./bench.sh && cd ../..

cd BFS/GBBS && bash ./bench.sh && cd ../..
cd BFS/ParSet && bash ./bench.sh && cd ../..

cd ColoringRound/GBBS && bash ./bench.sh && cd ../..
cd ColoringRound/ParSet && bash ./bench.sh && cd ../..

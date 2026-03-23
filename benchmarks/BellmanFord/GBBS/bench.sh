source ../../../benchmark_utils/scripts/config.sh
cd ../../../benchmark_utils/bazel
bazel build @gbbs_bellman_ford//:BellmanFord_main -c opt
for g in "${DENSEGRAPHS[@]}"; do
    #[[ $g == "friendster_sym" ]] && continue
    #[[ $g == "sd_arc_sym" ]] && continue
    numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}${g}_wgh8.adj"
done
for g in "${SPARSEGRAPHS[@]}"; do
    numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}${g}_wgh8.adj"
done
#for g in "${WEIGHTEDGRAPHS[@]}"; do
#    numactl -i all bazel-bin/external/gbbs_bellman_ford/BellmanFord_main -s "${ADJ_DIR}${g}.adj"
#done
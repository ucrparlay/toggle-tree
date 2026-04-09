licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_binary(
    name = "BFS_speedup_main",
    srcs = ["BFS.cc"],
    deps = [
        "@GBBS_BFS//:BFS",
        "@verify//:verify",
    ],
)

cc_binary(
    name = "KCore_speedup_main",
    srcs = ["KCore.cc"],
    deps = [
        "@GBBS_KCore//:KCore",
        "@verify//:verify",
    ],
)

cc_binary(
    name = "GraphColoring_speedup_main",
    srcs = ["GraphColoring.cc"],
    deps = [
        "@GBBS_Coloring//:GraphColoring",
        "@verify//:verify",
    ],
)

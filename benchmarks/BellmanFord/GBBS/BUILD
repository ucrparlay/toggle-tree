licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "BellmanFord",
    hdrs = ["BellmanFord.h"],
    deps = [
        "@gbbs//gbbs",
        "@benchmark_utils//:utils",
    ],
)

cc_binary(
    name = "BellmanFord_main",
    srcs = ["BellmanFord.cc"],
    deps = [":BellmanFord"],
)

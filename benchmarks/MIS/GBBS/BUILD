licenses(["notice"])
package(default_visibility = ["//visibility:public"],)

cc_library(
    name = "MIS",
    hdrs = ["MIS.h"],
    srcs = ["MIS.cc"], 
    deps = [
        "@gbbs//gbbs",
        "@gbbs//gbbs/helpers:speculative_for",
        "@benchmark_utils//:utils",
    ],
)

cc_binary(
    name = "MIS_main",
    srcs = ["MIS.cc"], 
    deps = [":MIS"],
)

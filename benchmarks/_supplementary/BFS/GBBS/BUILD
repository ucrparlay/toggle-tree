licenses(["notice"])
package(default_visibility = ["//visibility:public"],)

cc_library(
    name = "SparseBFS",
    hdrs = ["SparseBFS.h"],
    deps = [
        "@gbbs//gbbs",
        "@benchmark_utils//:utils",
    ],
)

cc_binary(
    name = "SparseBFS_main",
    srcs = ["SparseBFS.cc"],
    deps = [":SparseBFS"],
)

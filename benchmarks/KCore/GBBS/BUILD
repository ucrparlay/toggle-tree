licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "KCore",
    hdrs = ["KCore.h"],
    deps = [
        "@gbbs//gbbs",
        "@gbbs//gbbs:julienne",
        "@benchmark_utils//:utils",
    ],
)

cc_binary(
    name = "KCore_main",
    srcs = ["KCore.cc"],
    deps = [":KCore"],
)

licenses(["notice"])
package(default_visibility = ["//visibility:public"],)

cc_library(
    name = "BFS",
    hdrs = ["BFS.h"],
    deps = [
        "@gbbs//gbbs",
        "@verify//:verify",
    ],
)

cc_binary(
    name = "BFS_main",
    srcs = ["BFS.cc"],
    deps = [":BFS"],
)

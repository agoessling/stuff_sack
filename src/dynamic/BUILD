cc_library(
    name = "packing",
    hdrs = ["packing.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "type_descriptors",
    srcs = ["type_descriptors.cc"],
    hdrs = ["type_descriptors.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//src:uid_hash",
        "@yaml-cpp",
    ],
)

cc_library(
    name = "dynamic_types",
    hdrs = ["dynamic_types.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":packing",
        ":type_descriptors",
    ],
)

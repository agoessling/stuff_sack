py_library(
    name = "stuff_sack",
    srcs = ["stuff_sack.py"],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "stuff_sack_docs",
    srcs = ["stuff_sack_docs.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":c_stuff_sack",
        ":py_stuff_sack",
        ":stuff_sack",
    ],
)

py_binary(
    name = "c_stuff_sack",
    srcs = ["c_stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [":stuff_sack"],
)

py_library(
    name = "py_stuff_sack",
    srcs = ["py_stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":c_stuff_sack",
        ":libc_wrapper",
        ":stuff_sack",
    ],
)

py_library(
    name = "libc_wrapper",
    srcs = ["libc_wrapper.py"],
    visibility = ["//visibility:private"],
)

py_binary(
    name = "generate_python_lib",
    srcs = ["generate_python_lib.py"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "type_descriptors",
    srcs = ["type_descriptors.cc"],
    hdrs = ["type_descriptors.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@yaml-cpp",
    ],
)

cc_library(
    name = "logging",
    srcs = ["logging.c"],
    hdrs = ["logging.h"],
    visibility = ["//visibility:public"],
    alwayslink = True,
)

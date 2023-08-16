COPTS = [
    "-std=c17",
    "-Wall",
    "-Werror",
]

CXXOPTS = [
    "-std=c++17",
    "-Wall",
    "-Werror",
]

py_library(
    name = "utils",
    srcs = ["utils.py"],
    visibility = ["//visibility:public"],
)

py_library(
    name = "stuff_sack",
    srcs = ["stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":uid_hash-py",
        ":utils",
    ],
)

py_binary(
    name = "stuff_sack_docs",
    srcs = ["stuff_sack_docs.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":c_stuff_sack",
        ":cc_stuff_sack",
        ":py_stuff_sack",
        ":stuff_sack",
    ],
)

py_binary(
    name = "c_stuff_sack",
    srcs = ["c_stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":stuff_sack",
        ":utils",
    ],
)

py_binary(
    name = "cc_stuff_sack",
    srcs = ["cc_stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":c_stuff_sack",
        ":stuff_sack",
    ],
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
    name = "crc32",
    hdrs = ["crc32.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "uid_hash",
    srcs = ["uid_hash.cc"],
    hdrs = ["uid_hash.h"],
    copts = CXXOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":crc32",
    ],
)

cc_binary(
    name = "uid_hash-so",
    srcs = [
        "uid_hash.cc",
        "uid_hash.h",
    ],
    copts = CXXOPTS,
    linkshared = True,
    local_defines = ["PYTHON_LIB"],
    visibility = ["//visibility:public"],
    deps = [
        ":crc32",
    ],
)

py_library(
    name = "uid_hash-py",
    srcs = ["uid_hash.py"],
    data = [
        ":uid_hash-so",
    ],
    visibility = ["//visibility:public"],
    deps = ["@bazel_tools//tools/python/runfiles"],
)

cc_library(
    name = "logging",
    srcs = ["logging.c"],
    hdrs = ["logging.h"],
    copts = COPTS,
    visibility = ["//visibility:public"],
    alwayslink = True,
)

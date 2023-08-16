load("//tools:gen_stuff_sack.bzl", "all_stuff_sack")

exports_files(
    ["test_message_spec.yaml"],
    ["//visibility:public"],
)

all_stuff_sack(
    name = "test_message_def",
    c_alias_tag = "linalg-c",
    c_deps = [":external_c_vector3f"],
    c_includes = ["test/external_c_vector3f.h"],
    cc_alias_tag = "linalg-cpp",
    cc_deps = [":external_cc_vector3f"],
    cc_includes = ["test/external_cc_vector3f.h"],
    message_spec = "test_message_spec.yaml",
    visibility = ["//visibility:private"],
)

cc_test(
    name = "test_c_stuff_sack",
    srcs = ["test_c_stuff_sack.c"],
    visibility = ["//visibility:public"],
    deps = [
        ":external_c_vector3f",
        ":test_message_def-c",
        "@unity",
    ],
)

cc_test(
    name = "test_cc_stuff_sack",
    srcs = ["test_cc_stuff_sack.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_message_def-cc",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

py_test(
    name = "test_py_stuff_sack",
    srcs = ["test_py_stuff_sack.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_message_def-py",
    ],
)

py_binary(
    name = "gen_test_data",
    srcs = ["gen_test_data.py"],
    visibility = ["//visibility:private"],
    deps = [
        ":test_message_def-py",
    ],
)

genrule(
    name = "test_data",
    outs = ["test_data.bin"],
    cmd = "$(execpath :gen_test_data) $@",
    tools = [":gen_test_data"],
    visibility = [":__subpackages__"],
)

py_binary(
    name = "udp_sender",
    srcs = ["udp_sender.py"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_message_def-py",
    ],
)

cc_test(
    name = "test_crc32",
    srcs = ["test_crc32.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//src:crc32",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "external_c_vector3f",
    hdrs = ["external_c_vector3f.h"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "external_cc_vector3f",
    hdrs = ["external_cc_vector3f.h"],
    visibility = ["//visibility:private"],
)

load("//tools:gen_message_def.bzl", "gen_message_def")

exports_files(
    ["test_message_spec.yaml"],
    ["//visibility:public"],
)

gen_message_def(
    name = "test_message_def",
    message_spec = "test_message_spec.yaml",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_c_stuff_sack",
    srcs = ["test_c_stuff_sack.c"],
    visibility = ["//visibility:public"],
    deps = [
        ":test_message_def-c",
        "@unity",
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
    visibility = ["//visibility:public"],
    deps = [
        ":test_message_def-py",
    ],
)

genrule(
    name = "test_data",
    outs = ["test_data.bin"],
    cmd = "$(execpath :gen_test_data) $@",
    visibility = ["//visibility:public"],
    tools = [":gen_test_data"],
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

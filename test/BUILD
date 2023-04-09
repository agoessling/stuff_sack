load("//tools:gen_message_def.bzl", "gen_message_def")

gen_message_def(
    name = "test_message_def",
    c_deps = [
        ":linux_logging",
    ],
    message_spec = "test_message_spec.yaml",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "linux_logging",
    srcs = ["linux_logging.c"],
    hdrs = ["linux_logging.h"],
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

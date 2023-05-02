load("//tools:gen_message_def.bzl", "gen_message_def")

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
        "//src:logging",
        "@unity",
    ],
)

cc_test(
    name = "test_log_reader",
    srcs = ["test_log_reader.cc"],
    visibility = ["//visibility:public"],
    data = [
        "test_log.ss",
        "test_log_no_delimiter.ss",
    ],
    deps = [
        ":test_message_def-c",
        "//src:log_reader",
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

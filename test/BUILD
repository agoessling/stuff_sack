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

cc_test(
    name = "test_type_descriptors",
    srcs = ["test_type_descriptors.cc"],
    data = ["test_message_spec.yaml"],
    visibility = ["//visibility:public"],
    deps = [
        "//src:type_descriptors",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

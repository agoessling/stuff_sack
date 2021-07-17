load("//tools:gen_message_def.bzl", "gen_message_def")

gen_message_def(
    name = "test_message_def",
    message_spec = "test_message_spec.yaml",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_c_stuff_sack",
    srcs = ["test_c_stuff_sack.c"],
    deps = [
        ":test_message_def-c",
        "@unity//:unity",
    ],
    visibility = ["//visibility:public"],
)

py_test(
    name = "test_py_stuff_sack",
    srcs = ["test_py_stuff_sack.py"],
    deps = [
        ":test_message_def-py",
    ],
    visibility = ["//visibility:public"],
)

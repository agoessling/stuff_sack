load("//tools:gen_message_def.bzl", "gen_message_def")

gen_message_def(
    name = "test_message_def",
    message_spec = "test_message_spec.yaml",
)

cc_test(
    name = "test_c_stuff_sack",
    srcs = ["test_c_stuff_sack.c"],
    deps = [
        ":test_message_def",
        "@unity//:unity",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test_type_descriptors",
    srcs = ["test_type_descriptors.cc"],
    data = ["//test:test_message_spec.yaml"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/dynamic:type_descriptors",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "test_dynamic_types",
    srcs = ["test_dynamic_types.cc"],
    data = [
        "//test:test_data.bin",
        "//test:test_message_spec.yaml",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/dynamic:dynamic_types",
        "//src/dynamic:type_descriptors",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_test(
    name = "test_packing",
    srcs = ["test_packing.cc"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/dynamic:packing",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def stuff_sack_level_1_deps():
    http_archive(
        name = "unity",
        build_file = "@stuff_sack//tools:unity.BUILD",
        sha256 = "af00e4ecfcb7546cc8e6d39fe3770fa100b19e59ff63f7b6dcbd3010eac9f35f",
        strip_prefix = "Unity-2.5.1",
        url = "https://github.com/ThrowTheSwitch/Unity/archive/v2.5.1.zip",
    )

    http_archive(
        name = "rules_sphinx",
        strip_prefix = "rules_sphinx-0.1.0",
        sha256 = "526d6b2777ab6e94f7eef6cb9919a6ede5162f02772fa43e7514b211cad09c3e",
        url = "https://github.com/agoessling/rules_sphinx/archive/v0.1.0.zip",
    )

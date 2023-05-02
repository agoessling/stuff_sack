workspace(name = "stuff_sack")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Google test
http_archive(
    name = "gtest",
    sha256 = "353571c2440176ded91c2de6d6cd88ddd41401d14692ec1f99e35d013feda55a",
    strip_prefix = "googletest-release-1.11.0",
    url = "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip",
)

# unity
http_archive(
    name = "unity",
    build_file = "@stuff_sack//tools:unity.BUILD",
    sha256 = "af00e4ecfcb7546cc8e6d39fe3770fa100b19e59ff63f7b6dcbd3010eac9f35f",
    strip_prefix = "Unity-2.5.1",
    url = "https://github.com/ThrowTheSwitch/Unity/archive/v2.5.1.zip",
)

# bazel_lint
http_archive(
    name = "bazel_lint",
    sha256 = "85b8cab2998fc7ce32294d6473276ba70eea06b0eef4bce47de5e45499e7096f",
    strip_prefix = "bazel_lint-0.1.1",
    url = "https://github.com/agoessling/bazel_lint/archive/v0.1.1.zip",
)

load("@bazel_lint//bazel_lint:bazel_lint_first_level_deps.bzl", "bazel_lint_first_level_deps")

bazel_lint_first_level_deps()

load("@bazel_lint//bazel_lint:bazel_lint_second_level_deps.bzl", "bazel_lint_second_level_deps")

bazel_lint_second_level_deps()

# stuff_sack
load("@stuff_sack//tools:level_1_repositories.bzl", "stuff_sack_level_1_deps")

stuff_sack_level_1_deps()

load("@stuff_sack//tools:level_2_repositories.bzl", "stuff_sack_level_2_deps")

stuff_sack_level_2_deps()

load("@stuff_sack//tools:level_3_repositories.bzl", "stuff_sack_level_3_deps")

stuff_sack_level_3_deps()

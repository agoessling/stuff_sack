workspace(name = 'stuff_sack')

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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

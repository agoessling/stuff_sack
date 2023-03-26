load("@bazel_lint//bazel:buildifier.bzl", "buildifier")
load("@bazel_lint//cpp:clang.bzl", "clang_format")
load("@bazel_lint//python:pylint.bzl", "pylint")
load("@bazel_lint//python:yapf.bzl", "yapf")

buildifier(
    name = "format_bazel",
    srcs = ["WORKSPACE"],
    glob = [
        "**/*BUILD",
        "**/*.bzl",
    ],
    glob_exclude = [
        "bazel-*/**",
    ],
)

clang_format(
    name = "format_cc",
    glob = [
        "**/*.c",
        "**/*.cc",
        "**/*.h",
    ],
    glob_exclude = [
        "bazel-*/**",
    ],
    style_file = ".clang-format",
)

yapf(
    name = "format_python",
    glob = [
        "**/*.py",
    ],
    glob_exclude = [
        "bazel-*/**",
    ],
    style_file = ".style.yapf",
)

pylint(
    name = "lint_python",
    glob = [
        # TODO: Add compatibility over time.
    ],
    glob_exclude = [
        "bazel-*/**",
    ],
    rcfile = "pylintrc",
)

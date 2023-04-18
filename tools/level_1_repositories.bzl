load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def stuff_sack_level_1_deps():
    http_archive(
        name = "rules_sphinx",
        strip_prefix = "rules_sphinx-0.2.0",
        sha256 = "9de713bcc61dfad7234519a614291321cc6f4ebcd62beddfc1b0015fc373b483",
        url = "https://github.com/agoessling/rules_sphinx/archive/v0.2.0.zip",
    )

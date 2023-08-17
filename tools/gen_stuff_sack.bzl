load("@rules_sphinx//sphinx:defs.bzl", "sphinx_html")

COPTS = ["-std=c17", "-Wall", "-Werror"]
CXXOPTS = ["-std=c++17", "-Wall", "-Werror"]

def c_stuff_sack(name, message_spec, deps = None, includes = None, alias_tag = None, **kwargs):
    if deps == None:
        deps = []

    if includes == None:
        includes = []

    native.genrule(
        name = name + "-c-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath @stuff_sack//src:c_stuff_sack) --spec $(execpath {}) " +
               "--source $(execpath {}) --header $(execpath {}) --includes {}{}").format(
            message_spec,
            name + ".c",
            name + ".h",
            " ".join(["src/logging.h"] + includes),
            " --alias_tag {}".format(alias_tag) if alias_tag else "",
        ),
        tools = ["@stuff_sack//src:c_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name + "-c",
        srcs = [name + ".c"],
        hdrs = [name + ".h"],
        copts = COPTS,
        deps = deps + ["@stuff_sack//src:logging"],
        **kwargs
    )

def cc_stuff_sack(name, message_spec, deps = None, includes = None, alias_tag = None, **kwargs):
    if deps == None:
        deps = []

    if includes == None:
        includes = []

    native.genrule(
        name = name + "-cc-gen",
        srcs = [message_spec],
        outs = [
            name + ".cc",
            name + ".hpp",
        ],
        cmd = ("$(execpath @stuff_sack//src:cc_stuff_sack) --spec $(execpath {}) " +
               "--source $(execpath {}) --header $(execpath {}) --includes {}{}").format(
            message_spec,
            name + ".cc",
            name + ".hpp",
            " ".join(includes),
            " --alias_tag {}".format(alias_tag) if alias_tag else "",
        ),
        tools = ["@stuff_sack//src:cc_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name + "-cc",
        srcs = [name + ".cc"],
        hdrs = [name + ".hpp"],
        copts = CXXOPTS,
        deps = deps,
        **kwargs
    )

def py_stuff_sack(name, message_spec, **kwargs):
    native.genrule(
        name = name + "-py-c-gen",
        srcs = [message_spec],
        outs = [
            name + "-py.c",
            name + "-py.h",
        ],
        cmd = ("$(execpath @stuff_sack//src:c_stuff_sack) --spec $(execpath {}) " +
               "--source $(execpath {}) --header $(execpath {}) --includes {}").format(
            message_spec,
            name + "-py.c",
            name + "-py.h",
            " ".join(["src/logging.h"]),
        ),
        tools = ["@stuff_sack//src:c_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_binary(
        name = name + "-py-c-so",
        srcs = [
            name + "-py.c",
            name + "-py.h",
        ],
        copts = COPTS,
        deps = ["@stuff_sack//src:logging"],
        linkshared = True,
        visibility = ["//visibility:private"],
    )

    native.genrule(
        name = name + "-py-gen",
        srcs = [
            ":" + name + "-py-c-so",
            message_spec,
        ],
        outs = [
            name + ".py",
        ],
        cmd = ("$(execpath @stuff_sack//src:generate_python_lib) --spec $(rlocationpath {}) " +
               "--lib $(rlocationpath {}) --output $@").format(
            message_spec,
            ":" + name + "-py-c-so",
        ),
        tools = ["@stuff_sack//src:generate_python_lib"],
        visibility = ["//visibility:private"],
    )

    native.py_library(
        name = name + "-py",
        srcs = [name + ".py"],
        deps = [
            "@stuff_sack//src:py_stuff_sack",
            "@bazel_tools//tools/python/runfiles",
        ],
        data = [
            ":" + name + "-py-c-so",
            message_spec,
        ],
        **kwargs
    )

def doc_stuff_sack(name, message_spec, c_alias_tag = None, cc_alias_tag = None, **kwargs):
    native.genrule(
        name = name + "-doc-gen",
        srcs = [message_spec],
        outs = [
            name + "_index.rst",
            name + "_c.rst",
            name + "_cc.rst",
            name + "_py.rst",
        ],
        cmd = ("$(execpath @stuff_sack//src:stuff_sack_docs) --name {} --gen_dir $(GENDIR) " +
               "--spec $(execpath {}) --output_dir $(RULEDIR){}{}").format(
            name,
            message_spec,
            " --c_alias_tag {}".format(c_alias_tag) if c_alias_tag else "",
            " --cc_alias_tag {}".format(cc_alias_tag) if cc_alias_tag else "",
        ),
        tools = ["@stuff_sack//src:stuff_sack_docs"],
        visibility = ["//visibility:private"],
    )

    native.genrule(
        name = name + "-doc-conf",
        srcs = ["@stuff_sack//tools:conf.py.template"],
        outs = [name + "_conf.py"],
        cmd = "sed \"s/PROJECT_NAME/{}/g\" $< > $@".format(name),
        visibility = ["//visibility:private"],
    )

    sphinx_html(
        name = name + "-doc",
        config = name + "_conf.py",
        index = name + "_index.rst",
        srcs = [
            name + "_c.rst",
            name + "_cc.rst",
            name + "_py.rst",
            message_spec,
        ],
        **kwargs
    )

def all_stuff_sack(
        name,
        message_spec,
        c_deps = None,
        c_includes = None,
        c_alias_tag = None,
        cc_deps = None,
        cc_includes = None,
        cc_alias_tag = None,
        **kwargs):
    c_stuff_sack(name, message_spec, c_deps, c_includes, c_alias_tag, **kwargs)
    cc_stuff_sack(name, message_spec, cc_deps, cc_includes, cc_alias_tag, **kwargs)
    py_stuff_sack(name, message_spec, **kwargs)
    doc_stuff_sack(name, message_spec, c_alias_tag, cc_alias_tag, **kwargs)

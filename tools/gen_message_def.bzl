load("@rules_sphinx//sphinx:defs.bzl", "sphinx_html")

def gen_message_def(
        name,
        message_spec,
        c_deps = None,
        cc_deps = None,
        c_includes = None,
        cc_includes = None,
        **kwargs):
    if c_deps == None:
        c_deps = []

    if cc_deps == None:
        cc_deps = []

    if c_includes == None:
        c_includes = []

    if cc_includes == None:
        cc_includes = []

    native.genrule(
        name = name + "-c-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath @stuff_sack//src:c_stuff_sack) --spec $(execpath {}) " +
               "--header $(execpath {}) --c_file $(execpath {}) --includes src/logging.h {}").format(
            message_spec,
            name + ".h",
            name + ".c",
            " ".join(c_includes),
        ),
        tools = ["@stuff_sack//src:c_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name + "-c",
        srcs = [name + ".c"],
        hdrs = [name + ".h"],
        deps = c_deps + ["@stuff_sack//src:logging"],
        **kwargs
    )

    native.cc_binary(
        name = name + "-c-so",
        srcs = [
            name + ".c",
            name + ".h",
        ],
        deps = c_deps + ["@stuff_sack//src:logging"],
        linkshared = True,
        visibility = ["//visibility:private"],
    )

    native.genrule(
        name = name + "-py-gen",
        srcs = [
            ":" + name + "-c-so",
            message_spec,
        ],
        outs = [
            name + ".py",
        ],
        cmd = ("$(execpath @stuff_sack//src:generate_python_lib) --spec $(rlocationpath {}) " +
               "--lib $(rlocationpath {}) --output $@").format(
            message_spec,
            ":" + name + "-c-so",
        ),
        tools = ["@stuff_sack//src:generate_python_lib"],
        visibility = ["//visibility:private"],
    )

    native.genrule(
        name = name + "-cc-gen",
        srcs = [
            message_spec,
        ],
        outs = [
            name + ".cc",
            name + ".hpp",
        ],
        cmd = ("$(execpath @stuff_sack//src:cc_stuff_sack) --spec $(execpath {}) " +
               "--source $(execpath {}) --header $(execpath {}) --includes {}").format(
            message_spec,
            name + ".cc",
            name + ".hpp",
            " ".join(cc_includes),
        ),
        tools = ["@stuff_sack//src:cc_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name + "-cc",
        srcs = [name + ".cc"],
        hdrs = [name + ".hpp"],
        deps = cc_deps,
        **kwargs
    )

    native.py_library(
        name = name + "-py",
        srcs = [name + ".py"],
        deps = [
            "@stuff_sack//src:py_stuff_sack",
            "@bazel_tools//tools/python/runfiles",
        ],
        data = [
            ":" + name + "-c-so",
            message_spec,
        ],
        **kwargs
    )

    native.genrule(
        name = name + "-doc-gen",
        srcs = [message_spec],
        outs = [
            name + "_index.rst",
            name + "_c.rst",
            name + "_cc.rst",
            name + "_py.rst",
        ],
        cmd = ("$(execpath @stuff_sack//src:stuff_sack_docs) --name {}".format(name) +
               " --gen_dir $(GENDIR) --spec $(execpath {})".format(message_spec) +
               " --output_dir $(RULEDIR)"),
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
        name = name + "-docs",
        config = name + "_conf.py",
        index = name + "_index.rst",
        srcs = [
            name + "_c.rst",
            name + "_cc.rst",
            name + "_py.rst",
            message_spec,
        ],
    )

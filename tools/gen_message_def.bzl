load("@rules_sphinx//sphinx:defs.bzl", "sphinx_html")

def gen_message_def(name, message_spec, **kwargs):
    native.genrule(
        name = name + "-c-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath @stuff_sack//src:c_stuff_sack) --spec $(execpath {}) " +
               "--header $(execpath {}) --c_file $(execpath {})").format(
                   message_spec, name + ".h", name + ".c"),
        tools = ["@stuff_sack//src:c_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name + "-c",
        srcs = [name + ".c"],
        hdrs = [name + ".h"],
        **kwargs
    )

    native.filegroup(
        name = name + "-c-so",
        srcs = [":" + name + "-c"],
        output_group = "dynamic_library",
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
        cmd = ("$(execpath @stuff_sack//src:generate_python_lib) --spec $(rootpath {}) " +
               "--lib $(rootpath {}) --output $@").format(
                   message_spec, ":" + name + "-c-so"),
        tools = ["@stuff_sack//src:generate_python_lib"],
        visibility = ["//visibility:private"],
    )

    native.py_library(
        name = name + "-py",
        srcs = [name + ".py"],
        deps = [
            "@stuff_sack//src:py_stuff_sack",
        ],
        data = [
            ":" + name + "-c-so",
            message_spec
        ],
        **kwargs
    )

    native.genrule(
        name = name + "-doc-gen",
        srcs = [message_spec],
        outs = [
            name + "_index.rst",
            name + "_c.rst",
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
            name + "_py.rst",
            message_spec,
        ],
    )

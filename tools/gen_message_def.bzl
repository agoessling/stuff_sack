load("@rules_sphinx//sphinx:defs.bzl", "sphinx_html")

def gen_message_def(name, message_spec, **kwargs):
    native.genrule(
        name = name + "-c-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath //src:c_stuff_sack) --spec $(execpath {}) " +
               "--header $(execpath {}) --c_file $(execpath {})").format(
                   message_spec, name + ".h", name + ".c"),
        tools = ["//src:c_stuff_sack"],
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
        cmd = ("$(execpath //src:generate_python_lib) --spec $(rootpath {}) " +
               "--lib $(rootpath {}) --output $@").format(
                   message_spec, ":" + name + "-c-so"),
        tools = ["//src:generate_python_lib"],
        visibility = ["//visibility:private"],
    )

    native.py_library(
        name = name + "-py",
        srcs = [name + ".py"],
        deps = [
            "//src:py_stuff_sack",
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
        cmd = ("$(execpath //src:stuff_sack_docs) --name {} --gen_dir $(GENDIR)".format(name) +
               " --spec $(execpath {}) --output_dir $(RULEDIR)".format(message_spec)),
        tools = ["//src:stuff_sack_docs"],
    )

    native.genrule(
        name = name + "-doc-conf",
        srcs = ["//tools:conf.py.template"],
        outs = [name + "_conf.py"],
        cmd = "sed \"s/PROJECT_NAME/{}/g\" $< > $@".format(name),
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

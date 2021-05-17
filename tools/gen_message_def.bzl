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

def gen_message_def(name, message_spec, **kwargs):
    native.genrule(
        name = name + "-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath //src:c_message_pack) --spec $(location {}) " +
               "--header $(location {}) --c_file $(location {})").format(
                   message_spec, name + ".h", name + ".c"),
        tools = ["//src:c_message_pack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name,
        srcs = [name + ".c"],
        hdrs = [name + ".h"],
        **kwargs
    )

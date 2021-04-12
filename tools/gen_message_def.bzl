def gen_message_def(name, message_spec, **kwargs):
    native.genrule(
        name = name + "-gen",
        srcs = [message_spec],
        outs = [
            name + ".c",
            name + ".h",
        ],
        cmd = ("$(execpath @stuff_sack//src:c_stuff_sack) --spec $(location {}) " +
               "--header $(location {}) --c_file $(location {})").format(
                   message_spec, name + ".h", name + ".c"),
        tools = ["@stuff_sack//src:c_stuff_sack"],
        visibility = ["//visibility:private"],
    )

    native.cc_library(
        name = name,
        srcs = [name + ".c"],
        hdrs = [name + ".h"],
        **kwargs
    )

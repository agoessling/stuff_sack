py_library(
    name = "stuff_sack",
    srcs = ["stuff_sack.py"],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "stuff_sack_docs",
    srcs = ["stuff_sack_docs.py"],
    deps = [
        ":c_stuff_sack",
        ":py_stuff_sack",
        ":stuff_sack",
    ],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "c_stuff_sack",
    srcs = ["c_stuff_sack.py"],
    deps = [":stuff_sack"],
    visibility = ["//visibility:public"],
)

py_library(
    name = "py_stuff_sack",
    srcs = ["py_stuff_sack.py"],
    deps = [
        ":stuff_sack",
        ":c_stuff_sack",
    ],
    visibility = ["//visibility:public"],
)

py_binary(
    name = "generate_python_lib",
    srcs = ["generate_python_lib.py"],
    visibility = ["//visibility:public"],
)

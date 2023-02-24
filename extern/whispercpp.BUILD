load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

exports_files(
    [
        "ggml.h",
        "ggml.c",
        "whisper.h",
        "whisper.cpp",
    ],
    visibility = ["//visibility:public"],
)

HEADERS = [
    "ggml.h",
    "whisper.h",
]

cc_library(
    name = "ggml",
    srcs = [
        "ggml.c",
        "ggml.h",
    ],
    hdrs = HEADERS,
    copts = [
        "-fexceptions",
        "-Wunused-function",
    ],
)

cc_library(
    name = "whisper",
    srcs = ["whisper.cpp"] + HEADERS,
    hdrs = HEADERS,
    copts = ["-fexceptions"],
    deps = [":ggml"],
)

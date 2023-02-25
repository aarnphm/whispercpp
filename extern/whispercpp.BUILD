load("@rules_cc//cc:defs.bzl", "cc_library")
load("@bazel_skylib//lib:selects.bzl", "selects")

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
        "-O3",
        "-std=c11",
        "-fPIC",
        "-pthread",
    ] + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": ["-DGGML_USE_ACCELERATE"],
    }),
)

cc_library(
    name = "whisper",
    srcs = ["whisper.cpp"] + HEADERS,
    hdrs = HEADERS,
    copts = [
        "-fexceptions",
        "-Wunused-function",
        "-O3",
        "-std=c++11",
        "-fPIC",
        "-pthread",
    ],
    deps = [":ggml"],
)

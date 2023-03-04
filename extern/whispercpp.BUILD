load("@rules_cc//cc:defs.bzl", "cc_library")
load("@bazel_skylib//lib:selects.bzl", "selects")

package(default_visibility = ["//visibility:public"])

exports_files(
    [
        "ggml.h",
        "ggml.c",
        "whisper.h",
        "whisper.cpp",
    ] + glob(["examples/*.cpp"]) + glob(["examples/*.h"]),
    visibility = ["//visibility:public"],
)

HEADERS = [
    "ggml.h",
    "whisper.h",
]

EXAMPLE_HEADERS = [
    "examples/common-sdl.h",
    "examples/common.h",
    "examples/dr_wav.h",
]

CFLAGS = [
    "-fexceptions",
    "-Wunused-function",
    "-O3",
    "-std=c11",
    "-fPIC",
    "-pthread",
]

CXXFLAGS = [
    "-fexceptions",
    "-Wunused-function",
    "-O3",
    "-std=c++11",
    "-fPIC",
    "-pthread",
]

cc_library(
    name = "common",
    srcs = ["examples/common.cpp"],
    hdrs = EXAMPLE_HEADERS,
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
)

cc_library(
    name = "common-sdl",
    srcs = ["examples/common-sdl.cpp"],
    hdrs = EXAMPLE_HEADERS,
	deps = ["@com_github_libsdl_sdl2//:SDL_lib"],
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
)

cc_library(
    name = "ggml",
    srcs = ["ggml.c"],
    hdrs = HEADERS,
    copts = CFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
)

cc_library(
    name = "whisper",
    srcs = ["whisper.cpp"],
    hdrs = HEADERS + EXAMPLE_HEADERS,
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
    deps = [":ggml"],
)

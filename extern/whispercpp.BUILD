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
)

HEADERS = [
    "ggml.h",
    "whisper.h",
]

OPENVINO_HEADERS = glob(["openvino/*.h"])
OPENVINO_SOURCES = glob(["openvino/*.cpp"])

EXAMPLE_HEADERS = [
    "examples/common-sdl.h",
    "examples/common.h",
    "examples/dr_wav.h",
]

CFLAGS = [
    "-fexceptions",
    "-Wall",
    "-O3",
    "-std=c11",
    "-fPIC",
    "-pthread",
    "-DWHISPER_USE_OPENVINO",
    "-D_POSIX_C_SOURCE=200809L",
    "-D_USE_MATH_DEFINES",
    "-D_GNU_SOURCE",
]

CXXFLAGS = [
    "-fexceptions",
    "-Wall",
    "-O3",
    "-std=c++11",
    "-fPIC",
    "-pthread",
    "-DWHISPER_USE_OPENVINO",
    "-D_POSIX_C_SOURCE=200809L",
    "-D_USE_MATH_DEFINES",
    "-D_GNU_SOURCE",
]

cc_library(
    name = "common",
    srcs = ["examples/common.cpp"],
    hdrs = EXAMPLE_HEADERS,
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
    linkopts = selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
    }),
)

cc_library(
    name = "common-sdl",
    srcs = ["examples/common-sdl.cpp"],
    hdrs = EXAMPLE_HEADERS,
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
    linkopts = selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
    }),
    deps = ["@com_github_libsdl_sdl2//:SDL_lib"],
)

cc_library(
    name = "ggml",
    srcs = [
        "ggml.c",
    ],
    hdrs = HEADERS,
    copts = CFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
    linkopts = selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
    }),
)

cc_library(
    name = "whisper",
    srcs = ["whisper.cpp"] + OPENVINO_SOURCES,
    hdrs = HEADERS + EXAMPLE_HEADERS + OPENVINO_HEADERS,
    copts = CXXFLAGS + selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:linux_x86_64": [
            "-mavx",
            "-mavx2",
            "-mfma",
            "-mf16c",
            "-msse3",
        ],
    }),
    linkopts = selects.with_or({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-framework",
            "Accelerate",
        ],
    }),
    deps = [
        ":ggml",
	"@linux_openvino//:openvino_new_headers",
	"@linux_openvino//:openvino",
    ],
)

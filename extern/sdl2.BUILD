load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake", "configure_make")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

cc_library(
    name = "include",
    hdrs = glob(["include/**"]),
    strip_include_prefix = "include",
)

COPTS = [
    "-fPIC",
    "-pthread",
    "-O3",
]

cmake(
    name = "SDL",
    cache_entries = {
        "CMAKE_C_FLAGS": "-fPIC",
        "CMAKE_OSX_ARCHITECTURES": "x86_64;arm64",
        "CMAKE_OSX_DEPLOYMENT_TARGET": "10.14",
        "CMAKE_POSITION_INDEPENDENT_CODE": "ON",
        "CMAKE_INSTALL_LIBDIR": "lib",
        "SDL_SHARED": "OFF",
        "SDL_JOYSTICK": "OFF",
        "SDL_HAPTIC": "OFF",
        "SDL_COCOA": "OFF",
        "SDL_METAL": "OFF",
        "SDL_TEST": "OFF",
        "SDL_RENDER": "OFF",
        "SDL_VIDEO": "OFF",
    },
    copts = COPTS,
    lib_source = ":all_srcs",
    linkopts = select({
        "//conditions:default": [],
        "@bazel_tools//src/conditions:darwin": [
            "-Wl",
            "-framework",
            "CoreAudio,AudioToolbox",
        ],
    }),
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": [
            "libSDL2main.lib",
            "libSDL2.lib",
        ],
        "//conditions:default": [
            "libSDL2main.a",
            "libSDL2.a",
        ],
    }),
    targets = [
        "preinstall",
        "install",
    ],
)

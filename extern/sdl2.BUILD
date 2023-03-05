load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

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

configure_make(
    name = "SDL",
    configure_in_place = True,
    copts = COPTS,
    env = select({
        "@bazel_tools//src/conditions:darwin": {
            "AR": "",
            "OSX_DEPLOYMENT_TARGET": "10.14",
        },
        "//conditions:default": {},
    }),
    lib_name = "libSDL2",
    lib_source = ":all_srcs",
    out_shared_libs = select({
        "@bazel_tools//src/conditions:darwin": ["libSDL2.dylib"],
        "@bazel_tools//src/conditions:windows": ["libSDL2.dll"],
        "//conditions:default": ["libSDL2.so"],
    }),
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": ["libSDL2.lib"],
        "//conditions:default": ["libSDL2.a"],
    }),
    alwayslink = False,
)

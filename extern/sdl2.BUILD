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
    configure_options = ["--enable-shared=yes"],
    copts = COPTS,
    env = select({
        "@platforms//os:macos": {"AR": ""},
        "//conditions:default": {},
    }),
    lib_source = ":all_srcs",
    out_shared_libs = select({
        "@platforms//os:macos": ["libSDL2.dylib"],
        "//conditions:default": ["libSDL2.so"],
    }),
    alwayslink = True,
)

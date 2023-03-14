load("@com_github_bentoml_plugins//rules/py:python.bzl", py_test = "py_pytest")
load("@pypi//:requirements.bzl", "requirement")

exports_files(
    ["conftest.py"],
    visibility = ["//visibility:public"],
)

py_test(
    name = "export",
    size = "small",
    srcs = ["export_test.py"],
    data = [
        ":conftest.py",
        "//:api_cpp2py_export.so",
        "//samples:jfk.wav",
    ],
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
        requirement("numpy"),
        requirement("ffmpeg-python"),
    ],
)

py_test(
    name = "utils",
    size = "small",
    srcs = ["utils_test.py"],
    data = [
        ":conftest.py",
        "//:api_cpp2py_export.so",
    ],
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
    ],
)

py_test(
    name = "params",
    size = "small",
    srcs = ["params_export_test.py"],
    data = [
        ":conftest.py",
        "//:api_cpp2py_export.so",
    ],
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
    ],
)

py_test(
    name = "context",
    size = "enormous",
    srcs = ["context_export_test.py"],
    data = [
        ":conftest.py",
        "//:api_cpp2py_export.so",
        "//samples:jfk.wav",
    ] + glob(["models/*.bin"]),
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
        requirement("numpy"),
    ],
)

# NOTE: disable context test on CI, since it takes too long to run
test_suite(
    name = "ci",
    tests = [
        ":export",
        ":params",
        ":utils",
    ],
)

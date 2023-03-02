load("@com_github_bentoml_plugins//rules/py:python.bzl", py_test = "py_pytest")
load("@pypi//:requirements.bzl", "requirement")

py_test(
    name = "export",
    srcs = ["export_test.py"],
    data = [
        "//:api.so",
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
    srcs = ["utils_test.py"],
    data = ["//:api.so"],
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
    ],
)

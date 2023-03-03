load("@com_github_bentoml_plugins//rules/py:python.bzl", py_test = "py_pytest")
load("@pypi//:requirements.bzl", "requirement")

py_test(
    name = "explore",
    srcs = [
        "bindings/explore.py",
        "bindings/explore_test.py",
    ],
    data = ["//samples:jfk.wav"],
    deps = [
        "//src/whispercpp:whispercpp_lib",
        requirement("bazel-runfiles"),
        requirement("numpy"),
        requirement("ffmpeg-python"),
    ],
)

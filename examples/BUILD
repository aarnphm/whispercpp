load("@com_github_bentoml_plugins//rules/py:python.bzl", py_test = "py_pytest")

py_test(
    name = "explore",
    srcs = [
        "explore.py",
        "explore_test.py",
    ],
    data = ["//samples:jfk.wav"],
)

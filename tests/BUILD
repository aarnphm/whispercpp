load("@com_github_bentoml_plugins//rules/py:python.bzl", py_test = "py_pytest")

py_test(
    name = "export",
    srcs = ["export_test.py"],
    data = ["//samples:jfk.wav"],
)

py_test(
    name = "utils",
    srcs = ["utils_test.py"],
)

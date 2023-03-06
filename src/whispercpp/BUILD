load("@rules_python//python:defs.bzl", "py_library")

package(default_visibility = ["//:__subpackages__"])

exports_files(glob(["*.cc"]) + glob(["*.h"]) + glob(["*.py"]) + glob(["*.pyi"]))

py_library(
    name = "whispercpp_lib",
    srcs = glob(["*.py"]),
    data = [
        "//:api_cpp2py_export.so",
        "//:audio_cpp2py_export.so",
    ] + glob(["*.pyi"]),
)

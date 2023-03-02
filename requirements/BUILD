load("@python//3.10:defs.bzl", compile_pip_requirements_310 = "compile_pip_requirements")
load("@python//3.11:defs.bzl", compile_pip_requirements_311 = "compile_pip_requirements")
load("@python//3.8:defs.bzl", compile_pip_requirements_38 = "compile_pip_requirements")
load("@python//3.9:defs.bzl", compile_pip_requirements_39 = "compile_pip_requirements")

package(default_visibility = ["//visibility:__pkg__"])

# sets of *-requirements.in files to then be used by compile_pip_requirements
exports_files(glob(["*.txt"]))

extra_args = [
    "--allow-unsafe",
    "--resolver",
    "backtracking",
    "--no-annotate",
]

# This rule adds a convenient way to update the requirements.txt
# lockfile based on the requirements.in.
# Note that this rules will be used to distribute package with bazel.
compile_pip_requirements_310(
    name = "pypi_310",
    extra_args = extra_args,
    requirements_in = "//requirements:pypi.txt",
    requirements_txt = "//requirements:bazel-pypi-310.lock.txt",
    visibility = ["//visibility:__pkg__"],
)

compile_pip_requirements_311(
    name = "pypi_311",
    extra_args = extra_args,
    requirements_in = "//requirements:pypi.txt",
    requirements_txt = "//requirements:bazel-pypi-311.lock.txt",
    visibility = ["//visibility:__pkg__"],
)

compile_pip_requirements_38(
    name = "pypi_38",
    extra_args = extra_args,
    requirements_in = "//requirements:pypi.txt",
    requirements_txt = "//requirements:bazel-pypi-38.lock.txt",
    visibility = ["//visibility:__pkg__"],
)

compile_pip_requirements_39(
    name = "pypi_39",
    extra_args = extra_args,
    requirements_in = "//requirements:pypi.txt",
    requirements_txt = "//requirements:bazel-pypi-39.lock.txt",
    visibility = ["//visibility:__pkg__"],
)

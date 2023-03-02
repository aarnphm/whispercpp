workspace(name = "com_github_aarnphm_whispercpp")

load("//rules:deps.bzl", "internal_deps")

internal_deps()

load("@com_github_bentoml_plugins//rules:deps.bzl", plugins_repositories = "internal_deps")

plugins_repositories()

load("@rules_python//python:repositories.bzl", "py_repositories", "python_register_multi_toolchains")

py_repositories()

load("@rules_python//python/pip_install:repositories.bzl", "pip_install_dependencies")

pip_install_dependencies()

default_python_version = "3.10"

python_register_multi_toolchains(
    name = "python",
    default_version = default_python_version,
    python_versions = [
        "3.8",
        "3.9",
        "3.10",
        "3.11",
    ],
    register_coverage_tool = True,
)

load("@rules_python//python:pip.bzl", "pip_parse")
load("@python//3.10:defs.bzl", "interpreter")

pip_parse(
    name = "pypi",
    python_interpreter_target = interpreter,
    requirements_lock = "//requirements:bazel-pypi-310.lock.txt",
)

load("@pypi//:requirements.bzl", "install_deps")

install_deps()

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(name = "local_config_python")

load("//rules:python.bzl", "declare_python_abi")

declare_python_abi(
    name = "python_abi",
    python_version = "3",
)

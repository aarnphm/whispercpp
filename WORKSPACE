workspace(name = "com_github_aarnphm_whispercpp")

load("//rules:deps.bzl", "internal_deps")

internal_deps()

load("@com_github_bentoml_plugins//rules:deps.bzl", plugins_repositories = "internal_deps")

plugins_repositories()

# NOTE: external users wish to use BentoML workspace setup
# should always be loaded in this order.
load("@com_github_bentoml_plugins//rules:workspace0.bzl", "workspace0")

workspace0()

load("@com_github_bentoml_plugins//rules:workspace1.bzl", "workspace1")

workspace1()

load("@com_github_bentoml_plugins//rules:workspace2.bzl", "workspace2")

workspace2()

load("@rules_python//python:repositories.bzl", "py_repositories", "python_register_multi_toolchains")

py_repositories()

load("@rules_python//python/pip_install:repositories.bzl", "pip_install_dependencies")

pip_install_dependencies()

load("@rules_python//python:pip.bzl", "pip_parse")

pip_parse(
    name = "pypi",
    requirements_lock = "//requirements:bazel-pypi.lock.txt",
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

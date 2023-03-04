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

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

LLVM_VERSION = "15.0.6"

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_versions = {
        "": "15.0.6",
        "darwin-aarch64": "15.0.7",
        "darwin-x86_64": "15.0.7",
    },
    sha256 = {
        "": "38bc7f5563642e73e69ac5626724e206d6d539fbef653541b34cae0ba9c3f036",
        "darwin-aarch64": "867c6afd41158c132ef05a8f1ddaecf476a26b91c85def8e124414f9a9ba188d",
        "darwin-x86_64": "d16b6d536364c5bec6583d12dd7e6cf841b9f508c4430d9ee886726bd9983f1c",
    },
    strip_prefix = {
        "": "clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04",
        "darwin-aarch64": "clang+llvm-15.0.7-arm64-apple-darwin22.0",
        "darwin-x86_64": "clang+llvm-15.0.7-x86_64-apple-darwin21.0",
    },
    urls = {
        "": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz"],
        "darwin-aarch64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-arm64-apple-darwin22.0.tar.xz"],
        "darwin-x86_64": ["https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-x86_64-apple-darwin21.0.tar.xz"],
    },
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

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

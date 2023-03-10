workspace(name = "com_github_aarnphm_whispercpp")

load("//rules:deps.bzl", "internal_deps")

internal_deps()

load("@com_github_bentoml_plugins//rules:deps.bzl", "plugins_dependencies")

plugins_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies")

go_rules_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains")

go_register_toolchains(version = "1.19")

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

# If you use WORKSPACE.bazel, use the following line instead of the bare gazelle_dependencies():
# gazelle_dependencies(go_repository_default_config = "@//:WORKSPACE.bazel")
gazelle_dependencies()

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@build_bazel_rules_nodejs//:repositories.bzl", "build_bazel_rules_nodejs_dependencies")

build_bazel_rules_nodejs_dependencies()

load("@rules_nodejs//nodejs:repositories.bzl", "nodejs_register_toolchains")
load("@rules_nodejs//nodejs:yarn_repositories.bzl", "yarn_repositories")
load("@build_bazel_rules_nodejs//:index.bzl", "yarn_install")

nodejs_register_toolchains(
    name = "nodejs",
    node_version = "16.16.0",
)

yarn_repositories(
    name = "yarn",
    node_repository = "nodejs",
)

yarn_install(
    name = "npm",
    exports_directories_only = False,  # Required for ts_library
    frozen_lockfile = True,
    package_json = "//:package.json",
    package_path = "/",
    yarn = "@yarn//:bin/yarn",
    yarn_lock = "//:yarn.lock",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

load("@rules_python//python:pip.bzl", "pip_parse")

pip_parse(
    name = "pypi",
    requirements_lock = "//requirements:bazel-pypi.lock.txt",
)

load("@pypi//:requirements.bzl", pypi_deps = "install_deps")

pypi_deps()

pip_parse(
    name = "release",
    requirements_darwin = "//requirements/release:requirements_darwin.txt",
    requirements_lock = "//requirements/release:requirements.txt",
    requirements_windows = "//requirements/release:requirements_windows.txt",
)

load("@release//:requirements.bzl", release_deps = "install_deps")

release_deps()

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(
    name = "local_config_python",
)

load("//rules:python.bzl", "declare_python_abi")

declare_python_abi(
    name = "python_abi",
    python_version = "3",
)

"""Internal dependencies declaration, includes dependencies for examples, and gRPC clients."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

def internal_deps():
    """Load internal dependencies that are used within the BentoML projects."""

    # bentoml/plugins
    maybe(
        git_repository,
        name = "com_github_bentoml_plugins",
        remote = "https://github.com/bentoml/plugins.git",
        commit = "9da166c0d5af5543f6084bf3ae5223ea19f0e7ea",
        shallow_since = "1678069830 -0800",
    )
    # NOTE: uncomment the below line for debugging rules
    # change the path to absolute path
    # native.local_repository(
    #     name = "com_github_bentoml_plugins",
    #     path = "/Users/aarnphm/workspace/bentoml/ecosystem/",
    # )

    maybe(
        http_archive,
        name = "rules_cc",
        sha256 = "3d9e271e2876ba42e114c9b9bc51454e379cbf0ec9ef9d40e2ae4cec61a31b40",
        strip_prefix = "rules_cc-0.0.6",
        urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.6/rules_cc-0.0.6.tar.gz"],
    )

    maybe(
        http_archive,
        name = "pybind11_bazel",
        sha256 = "bc12ae921f521581894e30a5b156c226f6c5745cab0904fc4c16c6dbccc54173",
        strip_prefix = "pybind11_bazel-bebf131a6a29b34a6e18eab5ed75d899a44d60e7",
        urls = ["https://github.com/pybind/pybind11_bazel/archive/bebf131a6a29b34a6e18eab5ed75d899a44d60e7.zip"],
    )
    maybe(
        http_archive,
        name = "pybind11",
        build_file = "@pybind11_bazel//:pybind11.BUILD",
        sha256 = "5d8c4c5dda428d3a944ba3d2a5212cb988c2fae4670d58075a5a49075a6ca315",
        strip_prefix = "pybind11-2.10.3",
        urls = ["https://github.com/pybind/pybind11/archive/v2.10.3.tar.gz"],
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        sha256 = "74d544d96f4a5bb630d465ca8bbcfe231e3594e5aae57e1edbf17a6eb3ca2506",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.3.0/bazel-skylib-1.3.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_bazelbuild_buildtools",
        sha256 = "ae34c344514e08c23e90da0e2d6cb700fcd28e80c02e23e4d5715dddcb42f7b3",
        strip_prefix = "buildtools-4.2.2",
        urls = [
            "https://github.com/bazelbuild/buildtools/archive/refs/tags/4.2.2.tar.gz",
        ],
    )

    # whisper.cpp
    maybe(
        new_git_repository,
        name = "com_github_ggerganov_whisper",
        build_file = Label("//extern:whispercpp.BUILD"),
        init_submodules = True,
        recursive_init_submodules = True,
        remote = "https://github.com/ggerganov/whisper.cpp.git",
        commit = "09e90680072d8ecdf02eaf21c393218385d2c616",
        shallow_since = "1678217790 +0200",
    )

    http_archive(
        name = "com_github_libsdl_sdl2",
        build_file = Label("//extern:sdl2.BUILD"),
        sha256 = "e2ac043bd2b67be328f875043617b904a0bb7d277ba239fe8ac6b9c94b85cbac",
        strip_prefix = "SDL-dca3fd8307c2c9ebda8d8ea623bbbf19649f5e22",
        urls = ["https://github.com/libsdl-org/SDL/archive/dca3fd8307c2c9ebda8d8ea623bbbf19649f5e22.zip"],
    )

    git_repository(
        name = "rules_foreign_cc",
        remote = "https://github.com/bazelbuild/rules_foreign_cc",
        commit = "d33d862abb4ce3ba178547ef58c9fcb24cec38ca",
        shallow_since = "1677931962 +0000",
    )

    # Override python rules defined under @com_github_bentoml_plugins
    http_archive(
        name = "rules_python",
        sha256 = "b0a45e1fdaade7ac4975a9dfa08a170b78e209ba717946f7d397a75dc8a9fda6",
        strip_prefix = "rules_python-4e3d01c9f6a6cda0fb34b7c2961d6b1ef5313218",
        url = "https://github.com/bazelbuild/rules_python/archive/4e3d01c9f6a6cda0fb34b7c2961d6b1ef5313218.tar.gz",
    )

abi_bzl_template = """\
def python_abi():
    return "{python_abi}"
"""

def _get_python_bin(ctx):
    python_version = ctx.attr.python_version
    if python_version != "2" and python_version != "3":
        fail("python_version must be one of: '2', '3'")

    # we reuse the PYTHON_BIN_PATH environment variable from pybind11 so that the
    # ABI tag we detect is always compatible with the version of python that was
    # used for the build.
    python_bin = ctx.os.environ.get("PYTHON_BIN_PATH")
    if python_bin != None:
        return python_bin

    python_bin = ctx.which("python" + python_version)
    if python_bin != None:
        return python_bin

    fail("Failed to find python binary.")

def _get_python_abi(ctx, python_bin):
    result = ctx.execute([
        python_bin,
        "-c",
        "import platform;" +
        "assert platform.python_implementation() == 'CPython';" +
        "version = platform.python_version_tuple();" +
        "print(f'cp{version[0]}{version[1]}')",
    ])
    return result.stdout.splitlines()[0]

def _declare_python_abi_impl(ctx):
    python_bin = _get_python_bin(ctx)
    python_abi = _get_python_abi(ctx, python_bin)
    ctx.file("BUILD")
    ctx.file("abi.bzl", abi_bzl_template.format(python_abi = python_abi))

declare_python_abi = repository_rule(
    implementation = _declare_python_abi_impl,
    attrs = {
        "python_version": attr.string(mandatory = True),
    },
    local = True,
)

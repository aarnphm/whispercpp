from os import path
from pathlib import Path
from subprocess import check_output

from setuptools import setup
from wheel.bdist_wheel import bdist_wheel
from setuptools.command.build_py import build_py


def update_submodules(directory: str):
    check_output(["git", "init"])
    check_output(["git", "submodule", "sync", "--recursive"], cwd=directory)
    check_output(["git", "submodule", "update", "--init", "--recursive"], cwd=directory)


def compile_ext():
    wd = path.dirname(path.abspath(__file__))
    if not path.exists(path.join(wd, "src", "whispercpp", "api.so")):
        update_submodules(wd)
        print("Building pybind11 extension...")
        bazel_script = Path(wd) / "tools" / "bazel"
        check_output([bazel_script.__fspath__(), "run", "//:extensions"], cwd=wd)


class BdistWheel(bdist_wheel):
    def run(self):
        compile_ext()
        print("Building wheel...")
        bdist_wheel.run(self)


class BuildPy(build_py):
    def run(self):
        compile_ext()
        print("Installing package...")
        build_py.run(self)


setup(
    include_dirs=[
        "./extern/whispercpp",
        "./extern/pybind11/include",
        "./src/whispercpp/",
    ],
    cmdclass={"bdist_wheel": BdistWheel, "build_py": BuildPy},
)

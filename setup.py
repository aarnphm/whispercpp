import shutil
from os import path
from pathlib import Path
from subprocess import check_output

from setuptools import setup
from wheel.bdist_wheel import bdist_wheel
from setuptools.command.build_py import build_py


def update_submodules(directory: str):
    check_output(["git", "submodule", "sync", "--recursive"], cwd=directory)
    check_output(["git", "submodule", "update", "--init", "--recursive"], cwd=directory)


extension = "whispercpp/api.so"


def compile_ext():
    wd = path.dirname(path.abspath(__file__))
    target = path.join(wd, "src", "whispercpp", "api.so")
    if not path.exists(target):
        update_submodules(wd)
        print("Building pybind11 extension...")
        bazel_script = Path(wd) / "tools" / "bazel"
        check_output([bazel_script.__fspath__(), "build", ":api.so"], cwd=wd)
        out = path.join(
            check_output([bazel_script.__fspath__(), "info", "bazel-bin"])
            .decode("utf-8")
            .strip(),
            "api.so",
        )
        shutil.copy2(out, target)


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
    includ_dirs=[
        "./extern/whispercpp",
        "./extern/pybind11/include",
        "./src/whispercpp/",
    ],
    cmdclass={"bdist_wheel": BdistWheel, "build_py": BuildPy},
)

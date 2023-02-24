import shutil
from os import path
from pathlib import Path
from subprocess import check_output

import psutil
from setuptools import setup
from setuptools import Extension
from wheel.bdist_wheel import bdist_wheel


def update_submodules(directory: str):
    check_output(["git", "submodule", "sync", "--recursive"], cwd=directory)
    check_output(["git", "submodule", "update", "--init", "--recursive"], cwd=directory)


extension = "whispercpp/api.so"
package_data = {"whispercpp": ["py.typed", "api.so", "*.pyi"]}
ext_modules = []
if psutil.LINUX:
    ext_modules = [Extension("whispercpp.api", [extension])]


class BdistWheel(bdist_wheel):
    def run(self):
        wd = path.dirname(path.abspath(__file__))
        target = path.join(wd, "src", "whispercpp", "api.so")
        if not path.exists(target):
            update_submodules(wd)
            print("Building pybind11 extension...")
            bazel_script = Path(wd).parent.parent / "tools" / "bazel"
            check_output([bazel_script, "build", ":api.so"], cwd=wd)
            out = path.join(
                check_output([bazel_script, "info", "bazel-bin"])
                .decode("utf-8")
                .strip(),
                "api.so",
            )
            shutil.copy2(out, target)
        print("Building wheel...")
        bdist_wheel.run(self)


setup(
    include_package_data=True,
    packages=["whispercpp"],
    package_dir={"": "src"},
    package_data=package_data,
    cmdclass={"bdist_wheel": BdistWheel},
    ext_modules=ext_modules,
)

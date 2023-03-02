import sys
from os import path
from os import getcwd
from os import environ
from pathlib import Path

ROOT = Path(__file__).parent.parent


def test_main():
    # NOTE: If you are not using bazel, please ignore this.
    if environ.get("__BAZEL_TEST__") is not None:
        # NOTE: in bazel, the compiled so will be under the runfiles directory
        # probably need to write a rule to extend this C extension.
        import runfiles

        r = runfiles.Create({"RUNFILES_DIR": getcwd()})
        loc = r.Rlocation(".", source_repo="com_github_aarnphm_whispercpp")
        sys.path.insert(0, path.join(loc, "src"))

        # NOTE: this line is needed to make the test hermetic.
        import explore as explore

        # NOTE: This is a hack to add compiled api binary to the lib. This shouldn't
        # happen outside of tests. This makes the test hermetic
        import whispercpp

        sys.path.insert(0, loc)
        whispercpp.__dict__["api"] = __import__("api")
    else:
        import explore as explore

    p = ROOT / "samples" / "jfk.wav"
    explore.main([p.__fspath__()])
    assert (
        explore.get_model().context.full_get_segment_text(0)
        == " And so my fellow Americans ask not what your country can do for you ask what you can do for your country."
    )

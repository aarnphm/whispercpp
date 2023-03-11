import os
import sys
import shutil as s
from pathlib import Path

import pytest as p

# NOTE: this is used to make sure tests can be run hermetically.
if "BAZEL_TEST" in os.environ:
    import runfiles

    r = runfiles.Create({"RUNFILES_DIR": os.getcwd()})

    sys.path.insert(
        0,
        os.path.dirname(
            r.Rlocation("api_cpp2py_export.so", "com_github_aarnphm_whispercpp")
        ),
    )
    import api_cpp2py_export as wapi
    import audio_cpp2py_export as waudio

    sys.path.pop(0)

    sys.path.insert(0, r.Rlocation("src", "com_github_aarnphm_whispercpp"))
    import whispercpp as w

    object.__setattr__(w, "api", wapi)
    object.__setattr__(w, "audio", waudio)
else:
    import whispercpp as w

import explore as e

ROOT = Path(__file__).parent.parent.parent


@p.mark.skipif(not s.which("ffmpeg"), reason="ffmpeg not installed")
def test_main():
    p = ROOT / "samples" / "jfk.wav"
    e.main([p.__fspath__()])
    assert (
        e.get_model().context.full_get_segment_text(0)
        == " And so my fellow Americans ask not what your country can do for you ask what you can do for your country."
    )

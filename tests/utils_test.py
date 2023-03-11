from __future__ import annotations

import os
import sys
import typing as t

import pytest

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

if t.TYPE_CHECKING:
    pass


def test_invalid_load_module():
    with pytest.raises(ImportError):
        print(dir(w.utils.LazyLoader("invalid", globals(), "invalid")))
        del globals()["invalid"]


def test_available_audio_devices():
    # Right now, this function will always return a list
    # even there are no audio devices available.
    # we probably should just raise an error if there are no devices.
    w.utils.available_audio_devices()

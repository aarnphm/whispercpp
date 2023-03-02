from __future__ import annotations

import sys
import typing as t
import logging
from os import path
from os import getcwd
from os import environ

import pytest

if environ.get("__BAZEL_TEST__") is not None:
    # NOTE: in bazel, the compiled so will be under the runfiles directory
    # probably need to write a rule to extend this C extension.
    import runfiles

    r = runfiles.Create({"RUNFILES_DIR": getcwd()})
    loc = r.Rlocation(".", source_repo="com_github_aarnphm_whispercpp")
    sys.path.insert(0, path.join(loc, "src"))
    # NOTE: This is a hack to add compiled api binary to the lib. This shouldn't
    # happen outside of tests. This makes the test hermetic
    import whispercpp
    from whispercpp.utils import LazyLoader

    sys.path.insert(0, loc)
    whispercpp.__dict__["api"] = __import__("api")
else:
    from whispercpp.utils import LazyLoader

if t.TYPE_CHECKING:
    from _pytest.logging import LogCaptureFixture


def test_invalid_load_module():
    with pytest.raises(ImportError):
        print(dir(LazyLoader("invalid", globals(), "invalid")))
        del globals()["invalid"]


def test_lazy_warning(caplog: LogCaptureFixture):
    w = LazyLoader("w", globals(), "whispercpp", warning="test_warning")
    with caplog.at_level(logging.WARNING):
        w.Whisper.from_pretrained("tiny.en")
    assert "test_warning" in caplog.text

    assert "Whisper" in dir(w)

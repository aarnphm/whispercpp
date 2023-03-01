from __future__ import annotations

import typing as t
import logging

import pytest

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

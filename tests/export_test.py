from __future__ import annotations

import sys
import typing as t
from os import path
from os import getcwd
from os import environ
from pathlib import Path

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
    from whispercpp import Whisper
    from whispercpp.utils import LazyLoader

    sys.path.insert(0, loc)
    api = __import__("api")
    whispercpp.__dict__["api"] = api
else:
    from whispercpp import api
    from whispercpp import Whisper
    from whispercpp.utils import LazyLoader

if t.TYPE_CHECKING:
    import numpy as np
    import ffmpeg
    from numpy.typing import NDArray
else:
    np = LazyLoader("np", globals(), "numpy")
    ffmpeg = LazyLoader("ffmpeg", globals(), "ffmpeg")

ROOT = Path(__file__).parent.parent


def preprocess(file: Path, sample_rate: int = 16000) -> NDArray[np.float32]:
    try:
        y, _ = (
            ffmpeg.input(file.__fspath__(), threads=0)
            .output("-", format="s16le", acodec="pcm_s16le", ac=1, ar=sample_rate)
            .run(cmd=["ffmpeg", "-nostdin"], capture_stdout=True, capture_stderr=True)
        )
    except ffmpeg.Error as e:
        raise RuntimeError(f"Failed to load audio: {e.stderr.decode()}") from e

    return np.frombuffer(y, np.int16).flatten().astype(np.float32) / 32768.0


def test_invalid_models():
    with pytest.raises(RuntimeError):
        Whisper.from_pretrained("whisper_v0.1")


def test_forbid_init():
    with pytest.raises(RuntimeError):
        Whisper()


def test_from_pretrained():
    m = Whisper.from_pretrained("tiny.en")
    assert (
        " And so my fellow Americans ask not what your country can do for you ask what you can do for your country"
        == m.transcribe(preprocess(ROOT / "samples" / "jfk.wav"))
    )


def test_load_wav_file():
    np.testing.assert_almost_equal(
        preprocess(ROOT / "samples" / "jfk.wav"),
        api.load_wav_file(
            ROOT.joinpath("samples", "jfk.wav").resolve().__fspath__()
        ).mono,
    )


def test_transcribe_from_wav():
    m = Whisper.from_pretrained("tiny.en")
    assert m.transcribe_from_file(
        ROOT.joinpath("samples", "jfk.wav").resolve().__fspath__()
    ) == m.transcribe(preprocess(ROOT / "samples" / "jfk.wav"))

from __future__ import annotations

import typing as t
from pathlib import Path

import pytest

from whispercpp import Whisper
from whispercpp.utils import LazyLoader

if t.TYPE_CHECKING:
    import numpy as np
    import ffmpeg
    from numpy.typing import NDArray
    from _pytest.capture import CaptureFixture
else:
    np = LazyLoader("np", globals(), "numpy")
    ffmpeg = LazyLoader("ffmpeg", globals(), "ffmpeg")

ROOT = Path(__file__).parent.parent


def preprocess(file: Path, sample_rate=16000) -> NDArray[np.float32]:
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


def test_from_pretrained(capsys: CaptureFixture[str]):
    capsys.readouterr()
    m = Whisper.from_pretrained("tiny.en")
    assert (
        " And so my fellow Americans ask not what your country can do for you ask what you can do for your country"
        == m.transcribe(preprocess(ROOT / "samples" / "jfk.wav"))
    )

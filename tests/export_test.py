from __future__ import annotations

import shutil as s
import typing as t
from pathlib import Path

import pytest as p

import whispercpp as w

if t.TYPE_CHECKING:
    import numpy as np
    import ffmpeg
    from numpy.typing import NDArray
else:
    np = w.utils.LazyLoader("np", globals(), "numpy")
    ffmpeg = w.utils.LazyLoader("ffmpeg", globals(), "ffmpeg")

ROOT = Path(__file__).parent.parent

JFK_WAV = ROOT.joinpath("samples", "jfk.wav")


def preprocess(file: Path, sample_rate: int = 16000) -> NDArray[np.float32]:
    if not s.which("ffmpeg"):
        p.skip("ffmpeg not found, skipping this test.")
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
    with p.raises(RuntimeError):
        w.Whisper.from_pretrained("whisper_v0.1")
    with p.raises(RuntimeError):
        w.Whisper.from_params(
            "whisper_v0.1", w.api.Params.from_enum(w.api.SAMPLING_GREEDY)
        )


def test_forbid_init():
    with p.raises(RuntimeError):
        w.Whisper()


_EXPECTED = " And so my fellow Americans ask not what your country can do for you ask what you can do for your country"


@p.mark.skipif(not s.which("ffmpeg"), reason="ffmpeg not found, skipping this test.")
def test_from_pretrained():
    m = w.Whisper.from_pretrained("tiny.en")
    assert _EXPECTED == m.transcribe(preprocess(JFK_WAV))


@p.mark.skipif(not s.which("ffmpeg"), reason="ffmpeg not found, skipping this test.")
def test_from_params():
    m = w.Whisper.from_params("tiny.en", w.api.Params.from_enum(w.api.SAMPLING_GREEDY))
    assert _EXPECTED == m.transcribe(preprocess(JFK_WAV))


@p.mark.skipif(not s.which("ffmpeg"), reason="ffmpeg not found, skipping this test.")
def test_load_wav_file():
    np.testing.assert_almost_equal(
        preprocess(JFK_WAV),
        w.api.load_wav_file(str(JFK_WAV.resolve())).mono,
    )


def transcribe_strict():
    m = w.Whisper.from_pretrained("tiny.en", no_state=True)
    with p.raises(AssertionError, match="* and context is not initialized *"):
        m.transcribe_from_file(str(JFK_WAV.resolve()))


def test_transcribe_from_wav():
    m = w.Whisper.from_pretrained("tiny.en")
    assert (
        m.transcribe_from_file(
            ROOT.joinpath("samples", "jfk.wav").resolve().__fspath__()
        )
        == _EXPECTED
    )


def test_callback():
    def handleNewSegment(context: w.api.Context, n_new: int, text: list[str]):
        segment = context.full_n_segments() - n_new
        while segment < context.full_n_segments():
            text.append(context.full_get_segment_text(segment))
            print(text)
            segment += 1

    m = w.Whisper.from_pretrained("tiny.en")

    text = []
    m.params.on_new_segment(handleNewSegment, text)

    correct = m.transcribe(preprocess(ROOT / "samples" / "jfk.wav"))
    assert "".join(text) == correct

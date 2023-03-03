"""
Demonstrate how to use the C++ binding directly.
"""
from __future__ import annotations

import sys
import typing as t
import functools as f
from pathlib import Path

import whispercpp as w
from whispercpp.utils import LazyLoader

if t.TYPE_CHECKING:
    import numpy as np
    import ffmpeg
    from numpy.typing import NDArray
else:
    np = LazyLoader("np", globals(), "numpy")
    ffmpeg = LazyLoader("ffmpeg", globals(), "ffmpeg")

_model: w.Whisper | None = None


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


@f.lru_cache(maxsize=1)
def get_model() -> w.Whisper:
    global _model
    if _model is None:
        _model = w.Whisper.from_pretrained("tiny")
    return _model


def main(argv: list[str]) -> int:
    if len(argv) < 1:
        sys.stderr.write("Usage: explore.py <audio file>")
        sys.stderr.flush()
        return 1
    path = argv.pop(0)
    assert Path(path).exists()
    params = get_model().params
    assert _model is not None
    params.print_realtime = True
    _model.context.full_parallel(params, preprocess(Path(path)), 1)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

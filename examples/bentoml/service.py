from __future__ import annotations

import typing as t
from os import path
from os import environ

import bentoml
from bentoml._internal.types import FileLike
from bentoml._internal.utils import LazyLoader

if t.TYPE_CHECKING:
    import numpy as np
    import ffmpeg
    import whispercpp as w
    from numpy.typing import NDArray
else:
    ffmpeg = LazyLoader("ffmpeg", globals(), "ffmpeg")
    np = LazyLoader("np", globals(), "numpy")
    w = LazyLoader("w", globals(), "whispercpp")

SAMPLE_RATE = 16000


def preprocess(
    p: str | FileLike[t.Any], *, sample_rate: int = SAMPLE_RATE
) -> NDArray[np.float32]:
    if isinstance(path, str):
        p = path.abspath(path.realpath(p))
    try:
        y, _ = (
            ffmpeg.input(p, threads=0)
            .output("-", format="s16le", acodec="pcm_s16le", ac=1, ar=sample_rate)
            .run(cmd=["ffmpeg", "-nostdin"], capture_stdout=True, capture_stderr=True)
        )
    except ffmpeg.Error as e:
        raise RuntimeError(f"Failed to load audio: {e.stderr.decode()}") from e
    return np.frombuffer(y, np.int16).flatten().astype(np.float32) / 32768.0


class WhisperCppRunnable(bentoml.Runnable):
    SUPPORTED_RESOURCES = ("mps", "nvidia.com/gpu", "cpu")
    SUPPORTS_CPU_MULTI_THREADING = True

    def __init__(self):
        model_name = environ.get("GGML_MODEL", "tiny.en")
        self.model = w.Whisper.from_pretrained(model_name)

    @bentoml.Runnable.method(batchable=True, batch_dim=(0, 0))
    def transcribe(self, arr: NDArray[np.float32]):
        return self.model.transcribe(arr)


cpp_runner = bentoml.Runner(WhisperCppRunnable, max_batch_size=30)

svc = bentoml.Service("whispercpp_asr", runners=[cpp_runner])


@svc.api(
    input=bentoml.io.Text.from_sample("./samples/jfk.wav"),
    output=bentoml.io.Text(),
)
async def transcribe(input_file: str):
    return await cpp_runner.async_run(preprocess(input_file))

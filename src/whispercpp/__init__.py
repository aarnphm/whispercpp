from __future__ import annotations

import typing as t
from dataclasses import dataclass

from .utils import LazyLoader
from .utils import MODELS_URL
from .utils import download_model

if t.TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

    from . import api
else:
    api = LazyLoader("api", globals(), "whispercpp.api")
    del LazyLoader


@dataclass
class Whisper:
    def __init__(self, *args: t.Any, **kwargs: t.Any):
        raise RuntimeError(
            "Using '__init__()' is not allowed. Use 'from_pretrained()' instead."
        )

    if t.TYPE_CHECKING:
        # The following will be populated by from_pretrained.
        _ref: Whisper
        context: api.Context
        params: api.Params

    @staticmethod
    def from_pretrained(model_name: str, basedir: str | None = None):
        if model_name not in MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' is not a valid preconverted model. Choose one of {list(MODELS_URL)}"
            )
        _ref = object.__new__(Whisper)
        context = api.Context.from_file(download_model(model_name, basedir=basedir))
        params = api.Params.from_sampling_strategy(
            api.SamplingStrategies.from_strategy_type(api.SAMPLING_GREEDY)
        )
        params.print_progress = False
        params.print_realtime = False
        context.reset_timings()
        _ref.__dict__.update(locals())
        return _ref

    def transcribe(self, data: NDArray[np.float32], num_proc: int = 1):
        self.context.full_parallel(self.params, data, num_proc)
        return "".join(
            [
                self.context.full_get_segment_text(i)
                for i in range(self.context.full_n_segments())
            ]
        )

    def transcribe_from_file(self, filename: str, num_proc: int = 1):
        return self.transcribe(api.load_wav_file(filename).mono, num_proc)


__all__ = ["Whisper", "api"]

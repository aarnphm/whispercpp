from __future__ import annotations

import typing as _t
from dataclasses import dataclass as _dataclass

from .utils import LazyLoader
from .utils import MODELS_DIR
from .utils import MODELS_URL
from .utils import download_model

if _t.TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

    from . import api
else:
    api = LazyLoader("api", globals(), "whispercpp.api")
    del LazyLoader


@_dataclass
class Whisper:
    model: api.WhisperPreTrainedModel
    context: api.Context
    params: api.Params

    def __init__(self, *args: _t.Any, **kwargs: _t.Any):
        raise RuntimeError(
            "Using '__init__()' is not allowed. Use 'from_pretrained()' instead."
        )

    @classmethod
    def from_pretrained(cls, model_name: str):
        MODELS_DIR.mkdir(exist_ok=True)
        if model_name not in MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' is not a valid preconverted model. Choose one of {list(MODELS_URL)}"
            )
        _ref = object.__new__(cls)
        model = api.WhisperPreTrainedModel(str(download_model(model_name)))
        context = model.context
        params = model.params
        _ref.__dict__.update(locals())
        return _ref

    def transcribe(self, arr: NDArray[np.float32], num_proc: int = 1) -> str:
        if num_proc > 1:
            print(
                "num_proc > 1 may lead to worse performance and not threadsafe. Use with caution."
            )
        return self.model.transcribe(arr, num_proc)


__all__ = ["Whisper", "api"]

from __future__ import annotations

import typing as _t
from dataclasses import dataclass as _dataclass

from .utils import LazyLoader
from .utils import MODELS_DIR
from .utils import MODELS_URL
from .utils import download_model

if _t.TYPE_CHECKING:
    import api
    import numpy as np
    from numpy.typing import NDArray
else:
    api = LazyLoader("api", globals(), "whispercpp.api")
    del LazyLoader


@_dataclass
class Whisper:
    model: api.WhisperPreTrainedModel

    @classmethod
    def from_pretrained(cls, model_name: str):
        MODELS_DIR.mkdir(exist_ok=True)
        if model_name not in MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' is not a valid preconverted model. Choose one of {list(MODELS_URL)}"
            )
        return cls(api.WhisperPreTrainedModel(str(download_model(model_name))))

    def transcribe(self, arr: NDArray[np.float32], num_proc: int = 1) -> str:
        if num_proc > 1:
            print(
                "num_proc > 1 may lead to worse performance and not threadsafe. Use with caution."
            )
        return self.model.transcribe(arr, num_proc)


__all__ = ["Whisper", "api"]

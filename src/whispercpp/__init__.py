from __future__ import annotations

import typing as _t
import urllib.request as _r
from os import path as _path
from os import environ as _environ
from pathlib import Path as _Path
from dataclasses import dataclass as _dataclass

from .api import WhisperPreTrainedModel as _WhisperPreTrainedModel

if _t.TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray


_data_home = _environ.get("XDG_DATA_HOME", _path.expanduser("~/.local/share"))
_MODELS_DIR = _Path(_data_home) / "whispercpp"

_MODELS_URL = {
    model_type: f"https://huggingface.co/datasets/ggerganov/whisper.cpp/resolve/main/ggml-{model_type}.bin"
    for model_type in (
        "tiny.en",
        "tiny",
        "base.en",
        "base",
        "small.en",
        "small",
        "medium.en",
        "medium",
        "large-v1",
        "large",
    )
}


def _download_model(model_name: str) -> _Path:
    model_path = _MODELS_DIR / f"ggml-{model_name}.bin"
    if model_path.exists():
        return model_path
    print(f"Downloading model {model_name}. It may take a while...")
    _r.urlretrieve(_MODELS_URL[model_name], model_path)
    return model_path


@_dataclass
class Whisper:
    model: _WhisperPreTrainedModel

    @classmethod
    def from_pretrained(cls, model_name: str):
        _MODELS_DIR.mkdir(exist_ok=True)
        if model_name not in _MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' ijs not a valid preconverted model. Choose one of {list(_MODELS_URL)}"
            )
        return cls(_WhisperPreTrainedModel(str(_download_model(model_name))))

    def transcribe(self, arr: NDArray[np.float32], num_proc: int = 1) -> str:
        if num_proc > 1:
            print(
                "num_proc > 1 may lead to worse performance and not be threadsafe. Use with caution."
            )
        return self.model.transcribe(arr, num_proc)


__all__ = ["Whisper"]

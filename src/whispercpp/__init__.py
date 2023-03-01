from __future__ import annotations

import typing as t
from dataclasses import dataclass

from .utils import LazyLoader
from .utils import MODELS_URL
from .utils import download_model

if t.TYPE_CHECKING:
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
        _ref: api.WhisperPreTrainedModel
        context: api.Context
        params: api.Params

    @classmethod
    def from_pretrained(cls, model_name: str):
        if model_name not in MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' is not a valid preconverted model. Choose one of {list(MODELS_URL)}"
            )
        _ref = object.__new__(cls)
        _cpp_binding = api.WhisperPreTrainedModel(download_model(model_name))
        context = _cpp_binding.context
        params = _cpp_binding.params
        transcribe = _cpp_binding.transcribe
        del cls, _cpp_binding
        _ref.__dict__.update(locals())
        return _ref


__all__ = ["Whisper", "api"]

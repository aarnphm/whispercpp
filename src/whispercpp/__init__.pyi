from __future__ import annotations

from typing import Iterator
from typing import overload
from typing import TYPE_CHECKING

from . import api as api
from . import audio as audio
from . import utils as utils

if TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

__version__: str = ...
__version_tuple__: tuple[int, int, int, str] = ...

class Whisper:
    context: api.Context
    params: api.Params
    @overload
    def transcribe(self, data: NDArray[np.float32]) -> str: ...
    @overload
    def transcribe(
        self, data: NDArray[np.float32], num_proc: int = ..., strict: bool = ...
    ) -> str: ...
    @overload
    def transcribe_from_file(self, filename: str) -> str: ...
    @overload
    def transcribe_from_file(
        self, filename: str, num_proc: int = ..., strict: bool = ...
    ) -> str: ...
    @overload
    def stream_transcribe(self) -> Iterator[str]: ...
    @overload
    def stream_transcribe(
        self,
        *,
        length_ms: int = ...,
        device_id: int = ...,
        sample_rate: int | None = ...,
        step_ms: int = ...,
    ) -> Iterator[str]: ...
    @classmethod
    @overload
    def from_pretrained(cls, model_name: str) -> Whisper: ...
    @classmethod
    @overload
    def from_pretrained(
        cls, model_name: str, basedir: str | None = ..., no_state: bool = ...
    ) -> Whisper: ...
    @classmethod
    @overload
    def from_params(cls, model_name: str, params: api.Params) -> Whisper: ...
    @classmethod
    @overload
    def from_params(
        cls,
        model_name: str,
        params: api.Params,
        basedir: str | None = ...,
        no_state: bool = ...,
    ) -> Whisper: ...

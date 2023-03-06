from __future__ import annotations

from typing import overload
from typing import Generator
from typing import TYPE_CHECKING

from . import api as api
from . import audio as audio
from . import utils as utils

if TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

class Whisper:
    context: api.Context
    params: api.Params
    @overload
    def transcribe(self, data: NDArray[np.float32]) -> str: ...
    @overload
    def transcribe(self, data: NDArray[np.float32], num_proc: int = ...) -> str: ...
    @overload
    def transcribe_from_file(self, filename: str) -> str: ...
    @overload
    def transcribe_from_file(self, filename: str, num_proc: int = ...) -> str: ...
    @overload
    def stream_transcribe(self) -> Generator[str, None, list[str]]: ...
    @overload
    def stream_transcribe(
        self,
        *,
        length_ms: int = ...,
        device_id: int = ...,
        sample_rate: int | None = ...,
    ) -> Generator[str, None, list[str]]: ...
    @classmethod
    def from_pretrained(cls, model_name: str) -> Whisper: ...

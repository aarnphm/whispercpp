from typing import overload

import numpy as np
from numpy.typing import NDArray

from . import api as api

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
    async def capture_audio(self) -> None: ...
    @overload
    async def capture_audio(self, device_id: int | None = ...) -> None: ...
    @overload
    async def capture_audio(
        self, device_id: int | None = ..., sample_rate: int | None = ...
    ) -> None: ...
    @classmethod
    def from_pretrained(cls, model_name: str) -> Whisper: ...

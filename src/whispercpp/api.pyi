from __future__ import annotations

import enum
import typing as t

import numpy as np
from numpy.typing import NDArray

SAMPLE_RATE: int = ...
N_FFT: int = ...
N_MEL: int = ...
HOP_LENGTH: int = ...
CHUNK_SIZE: int = ...

class SamplingGreedyStrategy:
    best_of: int

class SamplingBeamSearchStrategy:
    beam_size: int
    patience: float

class StrategyType(enum.Enum):
    GREEDY = ...
    BEAM_SEARCH = ...

class SamplingStrategies:
    type: StrategyType
    greedy: SamplingGreedyStrategy
    beam_search: SamplingBeamSearchStrategy

# annotate the type of whisper_full_params
_CppFullParams = t.Any

class Params:
    fp: _CppFullParams

    num_threads: int
    num_max_text_ctx: int
    offset_ms: float
    duration_ms: float
    translate: bool
    no_context: bool
    single_segment: bool
    print_special: bool
    print_progress: bool
    print_realtime: bool
    print_timestamps: bool
    token_timestamps: bool
    timestamp_token_probability_threshold: float
    timestamp_token_sum_probability_threshold: float
    max_segment_length: int
    split_on_word: bool
    max_tokens: int
    speed_up: bool
    audio_ctx: int
    prompt_tokens: int
    prompt_num_tokens: int
    language: str
    suppress_blank: bool
    suppress_none_speech_tokens: bool
    temperature: float
    max_intial_timestamps: int
    length_penalty: float
    temperature_inc: float
    entropy_threshold: float
    logprob_threshold: float
    no_speech_threshold: float

    def set_tokens(self, tokens: list[str]) -> None: ...
    @staticmethod
    def from_sampling_strategy(sampling_strategies: SamplingStrategies) -> Params: ...

class Context:
    @staticmethod
    def from_file(filename: str) -> Context: ...
    @staticmethod
    def from_buffer(buffer: bytes) -> Context: ...
    def full(self, params: Params, data: NDArray[t.Any]) -> int: ...
    def full_parallel(
        self, params: Params, data: NDArray[t.Any], num_processor: int
    ) -> int: ...
    def full_get_segment_text(self, segment: int) -> str: ...
    def full_n_segments(self) -> int: ...
    def free(self) -> None: ...

class WhisperPreTrainedModel:
    context: Context
    params: Params
    @t.overload
    def __init__(self) -> None: ...
    @t.overload
    def __init__(self, path: str | bytes) -> None: ...
    def transcribe(self, arr: NDArray[np.float32], num_proc: int) -> str: ...

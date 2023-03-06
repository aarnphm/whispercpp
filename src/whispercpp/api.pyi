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

SAMPLING_GREEDY: StrategyType = ...
SAMPLING_BEAM_SEARCH: StrategyType = ...

class StrategyType(enum.Enum):
    SAMPLING_GREEDY = ...
    SAMPLING_BEAM_SEARCH = ...

class SamplingStrategies:
    type: StrategyType
    greedy: SamplingGreedyStrategy
    beam_search: SamplingBeamSearchStrategy
    @staticmethod
    def from_strategy_type(strategy_type: StrategyType) -> SamplingStrategies: ...

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
    def on_new_segment(
        self, callback: t.Callable[[Context, int, T], None], userdata: T
    ) -> None: ...

T = t.TypeVar("T")

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
    def full_get_token_data(self, segment: int, token: int) -> TokenData: ...
    def full_n_segments(self) -> int: ...
    def full_n_token(self, segment: int) -> int: ...
    def token_to_str(self, token: int) -> str: ...
    def free(self) -> None: ...
    def reset_timings(self) -> None: ...

class WavFile:
    mono: NDArray[np.float32]
    stereo: tuple[NDArray[np.float32], NDArray[np.float32]]

class TokenData:
    id: int
    tid: int
    p: float
    plog: float
    pt: float
    ptsum: float
    t0: int
    t1: int
    vlen: float

def load_wav_file(filename: str) -> WavFile: ...
def sdl_poll_events() -> bool: ...

class AudioCapture:
    transcript: list[str]
    def __init__(self, length_ms: int) -> None: ...
    @t.overload
    def init_device(self) -> bool: ...
    @t.overload
    def init_device(self, device_id: int) -> bool: ...
    @t.overload
    def init_device(self, device_id: int, sample_rate: int) -> bool: ...
    @staticmethod
    def list_available_devices() -> list[int]: ...
    @t.overload
    def stream_transcribe(
        self, context: Context, params: Params
    ) -> t.Iterator[str]: ...
    @t.overload
    def stream_transcribe(
        self, context: Context, params: Params, step_ms: int = ...
    ) -> t.Iterator[str]: ...
    def resume(self) -> bool: ...
    def pause(self) -> bool: ...
    def clear(self) -> bool: ...
    def retrieve_audio(self, ms: int, audio: NDArray[np.float32]) -> None: ...

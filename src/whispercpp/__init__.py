from __future__ import annotations

import sys
import typing as t
from dataclasses import dataclass

from . import utils

if t.TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

    from . import api
else:
    api = utils.LazyLoader("api", globals(), "whispercpp.api")


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
        if model_name not in utils.MODELS_URL:
            raise RuntimeError(
                f"'{model_name}' is not a valid preconverted model. Choose one of {list(utils.MODELS_URL)}"
            )
        _ref = object.__new__(Whisper)
        context = api.Context.from_file(
            utils.download_model(model_name, basedir=basedir)
        )
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

    def stream_transcribe(
        self,
        *,
        length_ms: int = 10000,
        device_id: int = 0,
        sample_rate: int | None = None,
    ) -> t.Generator[str, None, list[str]]:
        """
        Streaming transcription from microphone. Note that this function is blocking.

        Args:
            length_ms (int, optional): Length of audio to transcribe in milliseconds. Defaults to 10000.
            device_id (int, optional): Device ID of the microphone. Defaults to 0. Use ``
        """
        is_running = True

        if sample_rate is None:
            sample_rate = api.SAMPLE_RATE

        ac = api.AudioCapture(length_ms)
        if not ac.init_device(device_id, sample_rate):
            raise RuntimeError("Failed to initialize audio capture device.")

        try:
            while is_running:
                is_running = api.sdl_poll_events()
                if not is_running:
                    break
                ac.stream_transcribe(self.context, self.params)
        except KeyboardInterrupt:
            # handled from C++
            pass
        finally:
            yield from ac.transcript
            return ac.transcript


__all__ = ["Whisper", "api", "utils"]

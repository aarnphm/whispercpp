# whispercpp [![CI](https://github.com/aarnphm/whispercpp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/aarnphm/whispercpp/actions/workflows/ci.yml)

_Pybind11 bindings for
[whisper.cpp](https://github.com/ggerganov/whisper.cpp.git)_

## Quickstart

Install with pip:

```bash
pip install whispercpp
```

> NOTE: We will setup a hermetic toolchain for all platforms that doesn't have a
> prebuilt wheels, (which means you don't have to setup anything to install the
> Python package) which will take a bit longer to install. Pass `-vv` to `pip`
> to see the progress.

To use the latest version, install from source:

```bash
pip install git+https://github.com/aarnphm/whispercpp.git -vv
```

For local setup, initialize all submodules:

```bash
git submodule update --init --recursive
```

Build the wheel:

```bash
# Option 1: using pypa/build
python3 -m build -w

# Option 2: using bazel
./tools/bazel build //:whispercpp_wheel
```

Install the wheel:

```bash
# Option 1: via pypa/build
pip install dist/*.whl

# Option 2: using bazel
pip install $(./tools/bazel info bazel-bin)/*.whl
```

The binding provides a `Whisper` class:

```python
from whispercpp import Whisper

w = Whisper.from_pretrained("tiny.en")
```

Currently, the inference API is provided via `transcribe`:

```python
w.transcribe(np.ones((1, 16000)))
```

You can use any of your favorite audio libraries
([ffmpeg](https://github.com/kkroening/ffmpeg-python) or
[librosa](https://librosa.org/doc/main/index.html), or
`whispercpp.api.load_wav_file`) to load audio files into a Numpy array, then
pass it to `transcribe`:

```python
import ffmpeg
import numpy as np

try:
    y, _ = (
        ffmpeg.input("/path/to/audio.wav", threads=0)
        .output("-", format="s16le", acodec="pcm_s16le", ac=1, ar=sample_rate)
        .run(
            cmd=["ffmpeg", "-nostdin"], capture_stdout=True, capture_stderr=True
        )
    )
except ffmpeg.Error as e:
    raise RuntimeError(f"Failed to load audio: {e.stderr.decode()}") from e

arr = np.frombuffer(y, np.int16).flatten().astype(np.float32) / 32768.0

w.transcribe(arr)
```

You can also use the model `transcribe_from_file` for convience:

```python
w.transcribe_from_file("/path/to/audio.wav")
```

The Pybind11 bindings supports all of the features from whisper.cpp, that takes
inspiration from [whisper-rs](https://github.com/tazz4843/whisper-rs)

The binding can also be used via `api`:

```python
from whispercpp import api

# Binding directly fromn whisper.cpp
```

## Development

See [DEVELOPMENT.md](./DEVELOPMENT.md)

## APIs

### `Whisper`

1. `Whisper.from_pretrained(model_name: str) -> Whisper`

   Load a pre-trained model from the local cache or download and cache if
   needed.

   ```python
   w = Whisper.from_pretrained("tiny.en")
   ```

   The model will be saved to `$XDG_DATA_HOME/whispercpp` or
   `~/.local/share/whispercpp` if the environment variable is not set.

2. `Whisper.transcribe(arr: NDArray[np.float32], num_proc: int = 1)`

   Running transcription on a given Numpy array. This calls `full` from
   `whisper.cpp`. If `num_proc` is greater than 1, it will use `full_parallel`
   instead.

   ```python
   w.transcribe(np.ones((1, 16000)))
   ```

   To transcribe from a WAV file use `transcribe_from_file`:

   ```python
   w.transcribe_from_file("/path/to/audio.wav")
   ```

3. `Whisper.stream_transcribe(*, length_ms: int=..., device_id: int=..., num_proc: int=...) -> Iterator[str]`

   [EXPERIMENTAL] Streaming transcription. This calls `stream_` from
   `whisper.cpp`. The transcription will be yielded as soon as it's available.
   See [stream.py](./examples/stream/stream.py) for an example.

   > Note: The `device_id` is the index of the audio device. You can use
   > `whispercpp.api.available_audio_devices` to get the list of available audio
   > devices.

### `api`

`api` is a direct binding from `whisper.cpp`, that has similar API to
`whisper-rs`.

1. `api.Context`

   This class is a wrapper around `whisper_context`

   ```python
   from whispercpp import api

   ctx = api.Context.from_file("/path/to/saved_weight.bin")
   ```

   > Note: The context can also be accessed from the `Whisper` class via
   > `w.context`

2. `api.Params`

   This class is a wrapper around `whisper_params`

   ```python
   from whispercpp import api

   params = api.Params()
   ```

   > Note: The params can also be accessed from the `Whisper` class via
   > `w.params`

## Why not?

- [whispercpp.py](https://github.com/stlukey/whispercpp.py). There are a few key
  differences here:

  - They provides the Cython bindings. From the UX standpoint, this achieves the
    same goal as `whispercpp`. The difference is `whispercpp` use Pybind11
    instead. Feel free to use it if you prefer Cython over Pybind11. Note that
    `whispercpp.py` and `whispercpp` are mutually exclusive, as they also use
    the `whispercpp` namespace.
  - `whispercpp` provides similar APIs as
    [`whisper-rs`](https://github.com/tazz4843/whisper-rs), which provides a
    nicer UX to work with. There are literally two APIs (`from_pretrained` and
    `transcribe`) to quickly use whisper.cpp in Python.
  - `whispercpp` doesn't pollute your `$HOME` directory, rather it follows the
    [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html)
    for saved weights.

- Using `cdll` and `ctypes` and be done with it?

  - This is also valid, but requires a lot of hacking and it is pretty slow
    comparing to Cython and Pybind11.

## Examples

See [examples](./examples) for more information

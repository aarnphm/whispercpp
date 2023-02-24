# whispercpp

_Pybind11 bindings for
[whisper.cpp](https://github.com/ggerganov/whisper.cpp.git)_

## Quickstart

Initialize all submodules:

```bash
git submodule update --init --recursive
```

Build the wheel:

```bash
python3 -m build -w
```

Install the wheel:

```bash
pip install dist/*.whl
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

You can use [ffmpeg](https://github.com/kkroening/ffmpeg-python) or
[librosa](https://librosa.org/doc/main/index.html) to load audio files into a
Numpy array, then pass it to `transcribe`:

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

The Pybind11 bindings supports all of the features from whisper.cpp, that takes
inspiration from [whisper-rs](https://github.com/tazz4843/whisper-rs)

The binding can also be used via `api`:

```python
from whispercpp import api

# Binding directly fromn whisper.cpp
```

## TODO

- [ ] Release wheels for manylinux and MacOS.
- [ ] Added tests.

## Development

Recommended to use [bazel](https://bazel.build/). A `Makefile` is also provided
for convenience.

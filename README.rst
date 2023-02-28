==========
whispercpp
==========

*Pybind11 bindings for* `whisper.cpp <https://github.com/ggerganov/whisper.cpp.git>`_

Quickstart
~~~~~~~~~~

Install with pip:

.. code-block:: bash

    pip install whispercpp

To use the latest version, install from source:

.. code-block:: bash

    pip install git+https://github.com/aarnphm/whispercpp.git

For local setup, initialize all submodules:

.. code-block:: bash

    git submodule update --init --recursive

Build the wheel:

.. code-block:: bash

    # Option 1: using pypa/build
    python3 -m build -w

    # Option 2: using bazel
    ./tools/bazel build //:whispercpp_wheel

Install the wheel:

.. code-block:: bash

    # Option 1: via pypa/build
    pip install dist/*.whl

    # Option 2: using bazel
    pip install $(./tools/bazel info bazel-bin)/*.whl

The binding provides a ``Whisper`` class:

.. code-block:: python

    from whispercpp import Whisper

    w = Whisper.from_pretrained("tiny.en")

Currently, the inference API is provided via ``transcribe``:

.. code-block:: python

    w.transcribe(np.ones((1, 16000)))

You can use `ffmpeg <https://github.com/kkroening/ffmpeg-python>`_ or `librosa <https://librosa.org/doc/main/index.html>`_
to load audio files into a Numpy array, then pass it to ``transcribe``:

.. code-block:: python

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

The Pybind11 bindings supports all of the features from whisper.cpp.

The binding can also be used via ``api``:

.. code-block:: python

    from whispercpp import api

    ctx = api.Context.from_file("/path/to/saved_weight.bin")
    params = api.Params()

    ctx.full(arr, params)

Development
~~~~~~~~~~~

See `DEVELOPMENT.md <https://github.com/aarnphm/whispercpp/blob/main/DEVELOPMENT.md>`_

APIs
~~~~

``Whisper``
------------

1. ``Whisper.from_pretrained(model_name: str) -> Whisper``

   Load a pre-trained model from the local cache or download and cache if needed.

   .. code-block:: python

       w = Whisper.from_pretrained("tiny.en")

The model will be saved to ``$XDG_DATA_HOME/whispercpp`` or ``~/.local/share/whispercpp`` if the environment variable is
not set.

2. ``Whisper.transcribe(arr: NDArray[np.float32], num_proc: int = 1)``

   Running transcription on a given Numpy array. This calls ``full`` from ``whisper.cpp``. If ``num_proc`` is greater than 1,
   it will use ``full_parallel`` instead.

   .. code-block:: python

       w.transcribe(np.ones((1, 16000)))

``api``
-------

``api`` is a direct binding from ``whisper.cpp``, that has similar APIs to `whisper-rs <https://github.com/tazz4843/whisper-rs>`_.

1. ``api.Context``

   This class is a wrapper around ``whisper_context``

   .. code-block:: python

       from whispercpp import api

       ctx = api.Context.from_file("/path/to/saved_weight.bin")

   .. note::

       The context can also be accessed from the ``Whisper`` class via ``w.context``

2. ``api.Params``

   This class is a wrapper around ``whisper_params``

   .. code-block:: python

       from whispercpp import api

       params = api.Params()

   .. note::

       The params can also be accessed from the ``Whisper`` class via ``w.params``

Why not?
~~~~~~~~

* `whispercpp.py <https://github.com/stlukey/whispercpp.py>`_. There are a few key differences here:

  * They provides the Cython bindings. From the UX standpoint, this achieves the same goal as ``whispercpp``. The difference is ``whispercpp`` use Pybind11 instead. Feel free to use it if you prefer Cython over Pybind11. Note that ``whispercpp.py`` and ``whispercpp`` are mutually exclusive, as they also use the ``whispercpp`` namespace.

  * ``whispercpp`` doesn't pollute your ``$HOME`` directory, rather it follows the `XDG Base Directory Specification <https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html>`_ for saved weights.

* Using ``cdll`` and ``ctypes`` and be done with it?

  * This is also valid, but requires a lot of hacking and it is pretty slow comparing to Cython and Pybind11.

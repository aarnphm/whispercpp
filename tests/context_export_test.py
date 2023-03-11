from __future__ import annotations

import typing as t
import pathlib as p

import pytest

import whispercpp as w

if t.TYPE_CHECKING:
    import numpy as np
    from numpy.typing import NDArray

ROOT_TEST = p.Path(__file__).parent


@pytest.fixture(name="params")
def fixture_params() -> w.api.Params:
    return (
        w.api.Params.from_enum(w.api.SAMPLING_GREEDY).with_print_progress(False).build()
    )


@pytest.fixture(name="audio_file")
def fixture_audio_file() -> NDArray[np.float32]:
    return w.api.load_wav_file(
        ROOT_TEST.parent.joinpath("samples", "jfk.wav").resolve().__fspath__()
    ).mono


@pytest.mark.parametrize(
    "models", [path.__fspath__() for path in ROOT_TEST.joinpath("models").glob("*.bin")]
)
def test_from_file_with_state(
    models: str, params: w.api.Params, audio_file: NDArray[np.float32]
):
    context = w.api.Context.from_file(models)
    assert not context.full(params, audio_file)


@pytest.mark.parametrize(
    "models", [path.__fspath__() for path in ROOT_TEST.joinpath("models").glob("*.bin")]
)
def test_full_raise_exception(
    models: str, params: w.api.Params, audio_file: NDArray[np.float32]
):
    context = w.api.Context.from_file(models, no_state=True)
    with pytest.raises(RuntimeError):
        context.full(params, audio_file)


def test_full_with_init_state_manually(
    params: w.api.Params, audio_file: NDArray[np.float32]
):
    context = w.api.Context.from_file(
        ROOT_TEST.joinpath("models", "ggml-tiny.bin").__fspath__(), no_state=True
    )
    context.init_state()
    assert not context.full(params, audio_file)


@pytest.mark.parametrize(
    "models", [path.__fspath__() for path in ROOT_TEST.joinpath("models").glob("*.bin")]
)
def test_from_buffer(
    models: str, params: w.api.Params, audio_file: NDArray[np.float32]
):
    with open(models, "rb") as f:
        context = w.api.Context.from_buffer(f.read())
        assert not context.full(params, audio_file)

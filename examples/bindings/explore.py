"""Demonstrate how to use the C++ binding directly."""
from __future__ import annotations

import sys
import functools as f
from os import environ
from pathlib import Path

# NOTE: don't change this import, as it is currently used for a hack on test.
import whispercpp as w

_model: w.Whisper | None = None

_MODEL_NAME = environ.get("GGML_MODEL", "tiny")


@f.lru_cache(maxsize=1)
def get_model() -> w.Whisper:
    global _model
    if _model is None:
        _model = w.Whisper.from_pretrained(_MODEL_NAME)
    return _model


def main(argv: list[str]) -> int:
    """CLI entrypoint for demonstrating how to use the API binding directly."""
    if len(argv) < 1:
        sys.stderr.write("Usage: explore.py <audio file>\n")
        sys.stderr.flush()
        return 1

    path = argv.pop(0)
    assert Path(path).exists()
    params = get_model().params.with_print_realtime(True).build()
    assert _model is not None
    _model.context.full(params, w.api.load_wav_file(Path(path).__fspath__()).mono)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

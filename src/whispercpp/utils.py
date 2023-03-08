"""Utility functions and classes.

.. code-block:: python

    from whispercpp import utils

    print(utils.download_model("tiny.en"))

"""
from __future__ import annotations

import sys
import types
import typing as t
import logging
import importlib
import urllib.request as request
from os import path
from os import environ
from os import makedirs
from functools import lru_cache

logger = logging.getLogger(__name__)


_data_home = environ.get(
    "XDG_DATA_HOME",
    path.join(path.expanduser(path.expandvars("$HOME")), ".local", "share"),
)

MODELS_URL = {
    model_type: f"https://huggingface.co/datasets/ggerganov/whisper.cpp/resolve/main/ggml-{model_type}.bin"
    for model_type in (
        "tiny.en",
        "tiny",
        "base.en",
        "base",
        "small.en",
        "small",
        "medium.en",
        "medium",
        "large-v1",
        "large",
    )
}


def download_model(model_name: str, basedir: str | None = None) -> str:
    """Download a preconverted model from a given model name.

    Currently it doesn't support custom preconverted models yet. PRs are welcome.

    Args:
        model_name (str): Name of the preconverted model.
        basedir (str, optional): Base directory to store the model. Defaults to XDG_DATA_HOME/whispercpp

    Returns:
        The path to the downloaded model.
    """
    if basedir is None:
        basedir = _data_home
    models_dirs = path.join(basedir, "whispercpp")
    if not path.exists(models_dirs):  # pragma: no cover
        makedirs(models_dirs, exist_ok=True)

    model_path = path.join(models_dirs, f"ggml-{model_name}.bin")
    if not path.exists(model_path):
        print(f"Downloading model {model_name}. It may take a while...")
        request.urlretrieve(MODELS_URL[model_name], model_path)
    return model_path


class LazyLoader(types.ModuleType):
    """LazyLoader module borrowed from Tensorflow https://github.com/tensorflow/tensorflow/blob/v2.2.0/tensorflow/python/util/lazy_loader.py.

    This class adds "module caching".
    This will throw an exception if module cannot be imported.

    Lazily import a module, mainly to avoid pulling in large dependencies.
    `contrib`, and `ffmpeg` are examples of modules that are large and not always
    needed, and this allows them to only be loaded when they are used.
    """

    def __init__(
        self,
        local_name: str,
        parent_module_globals: dict[str, t.Any],
        name: str,
        warning: str | None = None,
        exc_msg: str | None = None,
        exc: type[Exception] = ImportError,
    ):
        """Create a lazy loaded module.

        Args:
            local_name (str): The name of the module in the parent's globals.
            parent_module_globals (dict[str, t.Any]): The parent's globals.
            name (str): The name of the module to be imported.
            warning (str, optional): A warning to be emitted when the module is loaded. Defaults to None.
            exc_msg (str, optional): An exception message to be raised when the module is loaded. Defaults to None.
            exc (type[Exception], optional): The exception to be raised when the module is loaded. Defaults to ImportError.

        Returns:
            LazyLoader: A lazy loaded module.
        """
        self._local_name = local_name
        self._parent_module_globals = parent_module_globals
        self._warning = warning
        self._exc_msg = exc_msg
        self._exc = exc
        self._module: types.ModuleType | None = None

        super().__init__(str(name))

    def _load(self) -> types.ModuleType:
        """Load the module and insert it into the parent's globals."""
        # Import the target module and insert it into the parent's namespace
        try:
            module = importlib.import_module(self.__name__)
            self._parent_module_globals[self._local_name] = module
            # The additional add to sys.modules ensures library is actually loaded.
            sys.modules[self._local_name] = module
        except ModuleNotFoundError as err:
            raise self._exc(f"{self._exc_msg} (reason: {err})") from None

        # Emit a warning if one was specified
        if self._warning:
            logger.warning(self._warning)
            # Make sure to only warn once.
            self._warning = None

        # Update this object's dict so that if someone keeps a reference to the
        #   LazyLoader, lookups are efficient (__getattr__ is only called on lookups
        #   that fail).
        self.__dict__.update(module.__dict__)
        return module

    def __getattr__(self, item: t.Any) -> t.Any:  # pragma: no cover
        """Lazy load the module on first access."""
        if self._module is None:
            self._module = self._load()
        return getattr(self._module, item)

    def __dir__(self) -> list[str]:
        """Return all module attributes. Lazy load the module on first access."""
        if self._module is None:
            self._module = self._load()
        return dir(self._module)


@lru_cache(maxsize=1)
def available_audio_devices() -> list[int]:
    """Returns a list of available audio devices on the system."""
    from whispercpp import audio  # type: ignore

    return audio.AudioCapture.list_available_devices()

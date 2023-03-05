import shutil as s
from pathlib import Path

import pytest as p
import explore as e

ROOT = Path(__file__).parent.parent.parent


@p.mark.skipif(not s.which("ffmpeg"), reason="ffmpeg not installed")
def test_main():
    p = ROOT / "samples" / "jfk.wav"
    e.main([p.__fspath__()])
    assert (
        e.get_model().context.full_get_segment_text(0)
        == " And so my fellow Americans ask not what your country can do for you ask what you can do for your country."
    )

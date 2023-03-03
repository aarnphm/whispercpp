from pathlib import Path

import explore as explore

ROOT = Path(__file__).parent.parent.parent


def test_main():
    p = ROOT / "samples" / "jfk.wav"
    explore.main([p.__fspath__()])
    assert (
        explore.get_model().context.full_get_segment_text(0)
        == " And so my fellow Americans ask not what your country can do for you ask what you can do for your country."
    )

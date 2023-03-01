from explore import main
from explore import Path
from explore import get_model

ROOT = Path(__file__).parent.parent


def test_main():
    p = ROOT / "samples" / "jfk.wav"
    main([p.__fspath__()])
    assert (
        get_model().context.full_get_segment_text(0)
        == " And so my fellow Americans ask not what your country can do for you ask what you can do for your country."
    )

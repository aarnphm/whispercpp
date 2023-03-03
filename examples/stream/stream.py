"""Some streaming examples."""

import sys
import typing as t

import whispercpp as w


def main(**kwargs: t.Any):
    iterator: t.Iterator[str] | None = None
    try:
        iterator = w.Whisper.from_pretrained(kwargs["model_name"]).stream_transcribe(
            length_ms=kwargs["length_ms"],
            device_id=kwargs["device_id"],
            sample_rate=kwargs["sample_rate"],
        )
    finally:
        assert iterator is not None, "Something went wrong!"
        sys.stderr.writelines(f"- {it}\n" for it in iterator)
        sys.stderr.write("Transcriptions:\n")
        sys.stderr.flush()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--model_name", default="tiny.en", choices=list(w.utils.MODELS_URL)
    )
    parser.add_argument(
        "--device_id", type=int, help="Choose the audio device", default=0
    )
    parser.add_argument(
        "--length_ms",
        type=int,
        help="Length of the audio buffer in milliseconds",
        default=10000,
    )
    parser.add_argument(
        "--sample_rate",
        type=int,
        help="Sample rate of the audio device",
        default=w.api.SAMPLE_RATE,
    )
    parser.add_argument(
        "--list_audio_devices",
        action="store_true",
        default=False,
        help="Show available audio devices",
    )

    args = parser.parse_args()

    if args.list_audio_devices:
        w.api.AudioCapture.list_available_devices()
        sys.exit(0)

    main(**vars(args))

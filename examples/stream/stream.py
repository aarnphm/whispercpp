"""
Some streaming exploration.
"""
from __future__ import annotations

import sys

import whispercpp as w

if __name__ == "__main__":
    try:
        w.Whisper.from_pretrained("tiny").stream_transcribe()
    except KeyboardInterrupt:
        sys.exit(0)

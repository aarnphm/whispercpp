"""
Some streaming exploration.
"""
from __future__ import annotations

import asyncio

import whispercpp as w


async def main(m: w.Whisper):
    await m.capture_audio()


if __name__ == "__main__":
    m = w.Whisper.from_pretrained("tiny")
    while True:
        asyncio.run(main(m))

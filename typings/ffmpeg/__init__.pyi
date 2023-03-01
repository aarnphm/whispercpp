from __future__ import annotations

import typing as t

class Error(Exception):
    stderr: bytes
    stdout: bytes

Stream = t.Any
Node = t.Any

class OutputNode(Node):
    def run(
        self,
        cmd: list[str],
        capture_stdout: bool,
        capture_stderr: bool,
        input: str | None = ...,
        quiet: bool = ...,
        overwrite_output: bool = ...,
    ) -> tuple[bytes, bytes]: ...

class FilterableStream(Stream):
    def output(self, filename: str, **kwargs: t.Any) -> OutputNode: ...

def input(filename: str, **kwargs: t.Any) -> FilterableStream: ...

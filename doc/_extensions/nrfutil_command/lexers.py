"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Pygments lexers that mark placeholders such as ``<serial_number>`` in ``nrfutil-command`` blocks.

Each lexer highlights like its base lexer and emits the placeholders as the ``PLACEHOLDER`` token,
which the HTML formatter renders as ``<span class="ge ge-Placeholder">``. The ``ge`` class makes
them italic in Pygments styles.
"""

from __future__ import annotations

import re
from collections.abc import Iterator
from typing import Any

from pygments.lexer import Lexer
from pygments.lexers.shell import BashLexer, BashSessionLexer
from pygments.lexers.special import TextLexer
from pygments.token import Token

PLACEHOLDER = Token.Generic.Emph.Placeholder
PLACEHOLDER_CLASS = "ge-Placeholder"
PLACEHOLDER_RE = re.compile(r"<([^<>\s][^<>]*?)>")


class _PlaceholderMixin:
    def get_tokens_unprocessed(self, text: str) -> Iterator[tuple[int, Any, str]]:
        # The base lexer splits a placeholder such as <hex file> into several tokens, so the
        # placeholders are found in the text and cut out of the tokens that overlap them
        spans = [match.span() for match in PLACEHOLDER_RE.finditer(text)]
        offset = 0
        for _index, ttype, value in super().get_tokens_unprocessed(text):
            start, end = offset, offset + len(value)
            offset = end
            for span_start, span_end in spans:
                if span_end <= start or span_start >= end:
                    continue
                if span_start > start:
                    yield start, ttype, text[start:span_start]
                start = max(start, span_start)
                yield start, PLACEHOLDER, text[start : min(end, span_end)]
                start = min(end, span_end)
            if start < end:
                yield start, ttype, text[start:end]


class ConsoleLexer(_PlaceholderMixin, BashSessionLexer):
    name = "nrfutil-command console"
    aliases = []


class ShellLexer(_PlaceholderMixin, BashLexer):
    name = "nrfutil-command shell"
    aliases = []


class NoneLexer(_PlaceholderMixin, TextLexer):
    name = "nrfutil-command none"
    aliases = []

    def __init__(self, **options):
        # Like the lexer that Sphinx uses for the none language
        options.setdefault("stripnl", False)
        super().__init__(**options)


# The languages of nrfutil-command blocks whose placeholders are marked, and their lexers
LEXERS: dict[str, type[Lexer]] = {
    "console": ConsoleLexer,
    "shell": ShellLexer,
    "bash": ShellLexer,
    "sh": ShellLexer,
    "none": NoneLexer,
    "text": NoneLexer,
}


def language(name: str) -> str:
    """The name under which the lexer of the language is registered with Sphinx."""
    return f"nrfutil-{name}"

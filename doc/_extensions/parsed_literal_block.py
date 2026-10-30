"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension wraps the ``pre`` elements of ``parsed-literal`` blocks in the same ``div``
elements that Pygments uses for code blocks. The theme then gives both the same width, padding and
border, and sphinx-copybutton adds its copy button to both.
"""

import re
from typing import Any

from sphinx.application import Sphinx

__version__ = "0.1.0"

LITERAL_BLOCK_RE = re.compile(
    r'<pre class="(?P<classes>[^"]*\bliteral-block\b[^"]*)"(?P<attrs>[^>]*)>(?P<body>.*?)</pre>',
    re.DOTALL,
)


def _wrap(match: re.Match) -> str:
    classes = " ".join(c for c in match["classes"].split() if c != "literal-block")
    class_attr = f' class="{classes}"' if classes else ""
    return (
        '<div class="highlight-parsed-literal notranslate"><div class="highlight">'
        f"<pre{class_attr}{match['attrs']}>{match['body']}</pre></div>\n</div>"
    )


def wrap_literal_blocks(
    app: Sphinx, pagename: str, templatename: str, context: dict[str, Any], doctree: Any
) -> None:
    body = context.get("body")
    if body and "literal-block" in body:
        context["body"] = LITERAL_BLOCK_RE.sub(_wrap, body)


def setup(app: Sphinx) -> dict[str, Any]:
    app.connect("html-page-context", wrap_literal_blocks)
    return {"version": __version__, "parallel_read_safe": True, "parallel_write_safe": True}

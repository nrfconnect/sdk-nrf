"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Shared Sphinx/docutils helpers for Matter memory documentation extensions.
"""

from __future__ import annotations

from collections.abc import Callable
from pathlib import Path
from typing import Any

from docutils import nodes
from docutils.statemachine import StringList
from memory_data import plain_tab_label
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

_EXTENSIONS_DIR = Path(__file__).parent
_STATIC_DIR = _EXTENSIONS_DIR / "static"


def parse_rst_inline(state, text: str, *, source_name: str) -> list[nodes.Node]:
    """Parse inline RST into a flat list of docutils nodes."""
    paragraph = nodes.paragraph()
    content = StringList([text], source_name)
    state.nested_parse(content, 0, paragraph)
    if len(paragraph) == 1 and isinstance(paragraph[0], nodes.paragraph):
        inner = paragraph[0]
        paragraph.remove(inner)
        paragraph.extend(inner.children)
    return list(paragraph.children)


def parse_rst_label(
    directive: SphinxDirective,
    text: str,
    *,
    css_class: str,
    source_name: str,
) -> nodes.Element:
    """Parse RST sample text into a labelled paragraph node."""
    label = nodes.paragraph()
    label["classes"] = [css_class]
    label.extend(parse_rst_inline(directive.state, text, source_name=source_name))
    return label


def append_rst_cell(state, entry: nodes.entry, text: str, *, source_name: str) -> None:
    """Append RST-parsed content to a table cell."""
    paragraph = nodes.paragraph()
    paragraph.extend(parse_rst_inline(state, text, source_name=source_name))
    entry += paragraph


def empty_data_note(message: str) -> list[nodes.Node]:
    """Return a single emphasis paragraph used when YAML data is missing."""
    note = nodes.paragraph()
    note += nodes.emphasis(text=message)
    return [note]


def board_tab_intro_lines(board: dict[str, Any]) -> list[str]:
    """Return indented RST lines for a board tab introduction."""
    return [f"      {line}" for line in board.get("tab_intro_rst", "").splitlines()]


def build_board_tab_lines(
    boards: list[dict[str, Any]],
    board_content_lines: Callable[[dict[str, Any]], list[str]],
) -> list[str]:
    """Build ``.. tabs::`` RST lines with per-board content."""
    lines = [".. tabs::", ""]
    for board in boards:
        lines.append(f"   .. group-tab:: {plain_tab_label(board['tab_title'])}")
        lines.append("")
        lines.extend(board_tab_intro_lines(board))
        lines.append("")
        lines.extend(board_content_lines(board))
        lines.append("")
    return lines


def build_board_tabs(
    directive: SphinxDirective,
    boards: list[dict[str, Any]],
    board_content_lines: Callable[[dict[str, Any]], list[str]],
    *,
    source_name: str,
    empty_message: str = "No memory usage data found.",
    env_attr: str | None = None,
    env_value: Callable[[dict[str, Any]], Any] | None = None,
) -> list[nodes.Node]:
    """Render boards as Sphinx ``.. tabs::`` group-tabs, one per board.

    ``board_content_lines`` returns the indented RST lines placed after each
    board's intro text. When ``env_attr`` is set, ``env_value(board)`` is stored
    on the build environment for a per-board subdirective to look up later.
    """
    if not boards:
        return empty_data_note(empty_message)

    if env_attr is not None:
        if env_value is None:
            raise ValueError("env_value is required when env_attr is set")
        setattr(
            directive.env,
            env_attr,
            {str(board["board_id"]): env_value(board) for board in boards},
        )

    container = nodes.container()
    directive.state.nested_parse(
        StringList(build_board_tab_lines(boards, board_content_lines), source_name),
        0,
        container,
    )
    return container.children


def subdirective_tab_lines(board: dict[str, Any], subdirective: str) -> list[str]:
    """Return RST lines for a per-board subdirective reference."""
    return [
        f"      .. {subdirective}::",
        f"         :board-id: {board['board_id']}",
    ]


def raw_html_tab_lines(board: dict[str, Any], html: str) -> list[str]:
    """Return RST lines embedding raw HTML for one board tab."""
    lines = ["      .. raw:: html", ""]
    lines.extend(f"         {html_line}" for html_line in html.splitlines())
    return lines


def register_static_css(app: Sphinx, css_filename: str) -> None:
    """Register extension static assets once."""
    static_path = _STATIC_DIR.as_posix()
    if static_path not in app.config.html_static_path:
        app.config.html_static_path.append(static_path)
    app.add_css_file(css_filename)


def extension_setup(
    app: Sphinx,
    *,
    version: str,
    css_filename: str,
    directives: dict[str, type],
) -> dict[str, Any]:
    """Register directives and static files for a memory extension."""
    for name, directive in directives.items():
        app.add_directive(name, directive)
    app.connect("builder-inited", lambda app: register_static_css(app, css_filename))
    return {
        "version": version,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

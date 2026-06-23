"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Sphinx extension for Matter sample memory requirement tables.
"""

from __future__ import annotations

from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from memory_data import (
    EMPTY_CELL,
    EXTERNAL_NVM_COLUMNS,
    INTERNAL_NVM_COLUMNS,
    RAM_SUBHEAD,
    board_has_external_nvm,
    header_label,
    load_memory_yaml,
    sample_table_cells,
    sample_title,
    sort_boards_by_internal_memory,
)
from memory_sphinx import (
    append_rst_cell,
    build_board_tabs,
    extension_setup,
    subdirective_tab_lines,
)
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

__version__ = "0.1.0"

ENV_BOARD_KEY = "matter_memory_table_boards"


def _append_text_cell(row: nodes.row, text: str, *, css_class: str = "memory-req-value") -> None:
    entry = nodes.entry()
    entry["classes"] = [css_class]
    entry += nodes.Text(text)
    row += entry


def _append_subhead_cells(row: nodes.row, labels: list[str]) -> None:
    for label in labels:
        entry = nodes.entry()
        entry["classes"] = ["memory-req-subhead"]
        entry += nodes.Text(label)
        row += entry


def build_memory_table_nodes(directive: SphinxDirective, board: dict[str, Any]) -> nodes.table:
    include_external = board_has_external_nvm(board)
    internal_count = len(INTERNAL_NVM_COLUMNS)
    external_count = len(EXTERNAL_NVM_COLUMNS) if include_external else 0
    total_cols = 1 + internal_count + external_count + 1
    external_single_column = include_external and external_count == 1

    table = nodes.table()
    table["classes"] = ["memory-req-table"]

    tgroup = nodes.tgroup(cols=total_cols)
    table += tgroup
    tgroup.extend([nodes.colspec(colwidth=28)] + [nodes.colspec(colwidth=10)] * (total_cols - 1))

    thead = nodes.thead()
    tgroup += thead

    group_row = nodes.row()
    thead += group_row

    sample_group = nodes.entry()
    sample_group["morerows"] = 1
    sample_group += nodes.Text("Sample")
    group_row += sample_group

    nvm_group = nodes.entry()
    nvm_group["morecols"] = internal_count - 1
    nvm_group["classes"] = ["memory-req-group-nvm"]
    nvm_group += nodes.Text(header_label(board, "nvm"))
    group_row += nvm_group

    if include_external:
        external_group = nodes.entry()
        if external_count > 1:
            external_group["morecols"] = external_count - 1
        external_group["classes"] = ["memory-req-group-external"]
        external_group += nodes.Text(header_label(board, "external"))
        group_row += external_group

    ram_group = nodes.entry()
    if external_single_column:
        ram_group["morerows"] = 1
    ram_group["classes"] = ["memory-req-group-ram"]
    ram_group += nodes.Text(header_label(board, "ram"))
    group_row += ram_group

    sub_row = nodes.row()
    thead += sub_row

    _append_subhead_cells(sub_row, [label for _, label, _ in INTERNAL_NVM_COLUMNS])

    if include_external:
        _append_subhead_cells(sub_row, [label for _, label in EXTERNAL_NVM_COLUMNS])

    if not external_single_column:
        _append_subhead_cells(sub_row, [RAM_SUBHEAD])

    tbody = nodes.tbody()
    tgroup += tbody

    ram_total_kb = board.get("ram_total_kb")
    ram_total = float(ram_total_kb) if ram_total_kb is not None else None
    for sample in board.get("samples") or []:
        row = nodes.row()
        tbody += row

        sample_entry = nodes.entry()
        sample_entry["classes"] = ["memory-req-sample"]
        append_rst_cell(
            directive.state,
            sample_entry,
            sample_title(sample),
            source_name="<memory_table>",
        )
        row += sample_entry

        cells = sample_table_cells(
            sample,
            include_external=include_external,
            ram_total_kb=ram_total,
        )
        for value in cells:
            css = "memory-req-empty" if value == EMPTY_CELL else "memory-req-value"
            _append_text_cell(row, value, css_class=css)

    return table


class MemoryTableBoard(SphinxDirective):
    """Render one board table (used inside generated board tabs)."""

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        "board-id": directives.unchanged_required,
    }

    def run(self) -> list[nodes.Node]:
        boards = getattr(self.env, ENV_BOARD_KEY, {})
        board = boards.get(self.options["board-id"])
        if board is None:
            return []
        return [build_memory_table_nodes(self, board)]


class MemoryTable(SphinxDirective):
    """Render memory requirements tables from a YAML data file."""

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        "file": directives.unchanged_required,
    }

    def run(self) -> list[nodes.Node]:
        data = load_memory_yaml(self, self.options["file"])
        return build_board_tabs(
            self,
            sort_boards_by_internal_memory(data.get("boards", [])),
            lambda board: subdirective_tab_lines(board, "memory-table-board"),
            source_name="<memory_table>",
            env_attr=ENV_BOARD_KEY,
            env_value=lambda board: board,
        )


def setup(app: Sphinx) -> dict[str, Any]:
    return extension_setup(
        app,
        version=__version__,
        css_filename="memory_table.css",
        directives={
            "memory-table": MemoryTable,
            "memory-table-board": MemoryTableBoard,
        },
    )

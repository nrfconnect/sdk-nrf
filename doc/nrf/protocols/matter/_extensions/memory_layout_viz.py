"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Sphinx extension for partition layout bar charts.

"""

from __future__ import annotations

import hashlib
import html
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from memory_data import load_memory_yaml, sort_boards_by_internal_memory
from memory_sphinx import build_board_tabs, extension_setup, raw_html_tab_lines
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

__version__ = "0.1.0"

_LAYOUT_COLORS = [
    "#2563eb",
    "#16a34a",
    "#7c3aed",
    "#ea580c",
    "#ca8a04",
    "#0d9488",
    "#db2777",
    "#475569",
    "#0891b2",
    "#65a30d",
]


_LAYOUT_FIXED_COLORS = {
    "padding": "#e2e8f0",
    "boot_partition": "#2563eb",
    "slot0_partition": "#16a34a",
    "slot1_partition": "#0369a1",
    "factory_data_partition": "#ea580c",
    "storage_partition": "#ca8a04",
}

TOOLTIP_ALIGN_START_THRESHOLD_PCT = 20.0


def _color_for_id(part_id: str) -> str:
    if part_id in _LAYOUT_FIXED_COLORS:
        return _LAYOUT_FIXED_COLORS[part_id]
    digest = hashlib.md5(part_id.encode()).hexdigest()
    return _LAYOUT_COLORS[int(digest[:8], 16) % len(_LAYOUT_COLORS)]


def _bytes_label(size_bytes: int) -> str:
    kb = size_bytes / 1024
    if abs(kb - round(kb)) < 0.05:
        return f"{int(round(kb))} kB"
    return f"{kb:.2f} kB"


def _hex_label(size_bytes: int) -> str:
    return f"0x{size_bytes:x}"


def _display_widths(sizes_bytes: list[int]) -> list[float]:
    """Map partition sizes to bar widths proportional to byte size."""
    if not sizes_bytes:
        return []
    total = sum(max(size, 0) for size in sizes_bytes) or 1
    return [100.0 * max(size, 0) / total for size in sizes_bytes]


def _part_tooltip(part: dict[str, Any]) -> str:
    size_bytes = int(part["size_bytes"])
    return f'{part["label"]}: {_bytes_label(size_bytes)} ({_hex_label(size_bytes)})'


def _tooltip_align_class(bar_start_pct: float, width_pct: float) -> str:
    center_pct = bar_start_pct + width_pct / 2.0
    if center_pct < TOOLTIP_ALIGN_START_THRESHOLD_PCT:
        return " memory-layout-segment-tooltip-start"
    return ""


def _render_layout_segment(
    part: dict[str, Any],
    display_width_pct: float,
    depth: int = 0,
    bar_start_pct: float = 0.0,
    bar_width_pct: float | None = None,
) -> str:
    if bar_width_pct is None:
        bar_width_pct = display_width_pct
    color = _color_for_id(part["id"])
    tooltip = html.escape(_part_tooltip(part), quote=True)
    children = part.get("children") or []
    tooltip_class = _tooltip_align_class(bar_start_pct, bar_width_pct)

    if children:
        child_widths = _display_widths([int(child["size_bytes"]) for child in children])
        child_offset_pct = 0.0
        child_segments = []
        for child, child_width in zip(children, child_widths, strict=True):
            child_bar_start = bar_start_pct + (child_offset_pct / 100.0) * bar_width_pct
            child_bar_width = (child_width / 100.0) * bar_width_pct
            child_segments.append(
                _render_layout_segment(
                    child,
                    child_width,
                    depth + 1,
                    bar_start_pct=child_bar_start,
                    bar_width_pct=child_bar_width,
                )
            )
            child_offset_pct += child_width
        inner = f'<div class="memory-layout-subbar">{"".join(child_segments)}</div>'
        segment_class = "memory-layout-segment memory-layout-segment-nested"
    else:
        inner = f'<span class="memory-layout-fill" style="background:{color}"></span>'
        segment_class = "memory-layout-segment"

    return (
        f'<div class="{segment_class}{tooltip_class} memory-layout-depth-{depth}" '
        f'style="flex:0 0 {display_width_pct:.4f}%" data-tooltip="{tooltip}">'
        f"{inner}</div>"
    )


def _render_region_legend(
    parts: list[dict[str, Any]],
    collected: list[str] | None = None,
    seen_ids: set[str] | None = None,
) -> list[str]:
    if collected is None:
        collected = []
    if seen_ids is None:
        seen_ids = set()

    for part in parts:
        part_id = part["id"]
        if part_id in seen_ids:
            if part.get("children"):
                _render_region_legend(part["children"], collected, seen_ids)
            continue
        seen_ids.add(part_id)
        swatch = (
            f'<span class="memory-viz-swatch" style="background:{_color_for_id(part_id)}"></span>'
        )
        size_bytes = int(part["size_bytes"])
        label = f'{html.escape(part["label"])} ({_bytes_label(size_bytes)})'
        collected.append(f'<span class="memory-viz-legend-item">{swatch}{label}</span>')
        if part.get("children"):
            _render_region_legend(part["children"], collected, seen_ids)
    return collected


def _render_region_html(region: dict[str, Any]) -> str:
    partitions = region.get("partitions", [])
    total_bytes = int(region["total_bytes"])
    legend_items = _render_region_legend(partitions)
    legend = (
        f'<div class="memory-viz-legend memory-layout-legend">{"".join(legend_items)}</div>'
        if legend_items
        else ""
    )
    display_widths = _display_widths([int(part["size_bytes"]) for part in partitions])
    segments = []
    bar_start_pct = 0.0
    for part, width_pct in zip(partitions, display_widths, strict=True):
        segments.append(
            _render_layout_segment(
                part,
                width_pct,
                bar_start_pct=bar_start_pct,
                bar_width_pct=width_pct,
            )
        )
        bar_start_pct += width_pct
    segments_html = "".join(segments)
    note = ""
    if region.get("address_note_rst"):
        note = f'<p class="memory-layout-note">{html.escape(region["address_note_rst"])}</p>'

    return (
        f'<div class="memory-layout-region" data-region="{html.escape(region["id"], quote=True)}">'
        f'<div class="memory-layout-region-title">'
        f'{html.escape(region["title"])} '
        f'(size: {_hex_label(total_bytes)} = {_bytes_label(total_bytes)})'
        f"</div>"
        f"{note}"
        f"{legend}"
        f'<div class="memory-layout-bar">{segments_html}</div>'
        f"</div>"
    )


def _render_board_layout_html(board: dict[str, Any]) -> str:
    regions = "".join(_render_region_html(region) for region in board.get("regions", []))
    return (
        f'<div class="memory-layout-board" '
        f'data-board="{html.escape(board["board_id"], quote=True)}">'
        f"{regions}</div>"
    )


class MemoryLayouts(SphinxDirective):
    """Render reference memory layout charts from a YAML data file."""

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
            lambda board: raw_html_tab_lines(board, _render_board_layout_html(board)),
            source_name="<memory_layout>",
            empty_message="No memory layout data found.",
        )


def setup(app: Sphinx) -> dict[str, Any]:
    return extension_setup(
        app,
        version=__version__,
        css_filename="memory_layout.css",
        directives={"memory-layouts": MemoryLayouts},
    )

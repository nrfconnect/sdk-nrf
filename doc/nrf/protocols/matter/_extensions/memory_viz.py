"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Sphinx extension for sample memory bar charts.

"""

from __future__ import annotations

import html
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from memory_data import (
    board_external_total_kb,
    collect_board_partitions,
    kb_label,
    legend_threshold_kb,
    load_memory_yaml,
    plain_tab_label,
    region_title,
    sample_title,
    sort_boards_by_internal_memory,
)
from memory_sphinx import (
    build_board_tabs,
    extension_setup,
    parse_rst_label,
    subdirective_tab_lines,
)
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective

__version__ = "0.1.0"

ENV_BOARD_KEY = "matter_memory_usage_boards"

PARTITION_COLORS = {
    "boot": ("#2563eb", "#93c5fd"),
    "slot0": ("#16a34a", "#bbf7d0"),
    "slot1": ("#0369a1", "#7dd3fc"),
    "tfm": ("#7c3aed", "#ddd6fe"),
    "factory_data": ("#ea580c", "#fed7aa"),
    "storage": ("#ca8a04", "#fde68a"),
    "tfm_storage": ("#0d9488", "#99f6e4"),
    "padding": ("#94a3b8", "#e2e8f0"),
    "ram": ("#0891b2", "#a5f3fc"),
    "unused": ("#e2e8f0", "#f8fafc"),
}

SPLIT_USAGE_IDS = frozenset({"boot", "slot0", "slot1", "ram"})
ALWAYS_VISIBLE_BAR_IDS = frozenset(
    {"boot", "tfm", "slot0", "slot1", "factory_data", "storage", "tfm_storage", "padding"}
)
NVM_LEGEND_ORDER = (
    "boot",
    "tfm",
    "slot0",
    "factory_data",
    "storage",
    "tfm_storage",
)
EXTERNAL_NVM_LEGEND_ORDER = (
    "slot1",
    "padding",
)
TOOLTIP_ALIGN_START_THRESHOLD_PCT = 20.0


def _partition_colors(part_id: str) -> tuple[str, str]:
    return PARTITION_COLORS.get(part_id, PARTITION_COLORS["unused"])


def _used_free_pcts(used_kb: float, size_kb: float) -> tuple[float, float]:
    used_kb = min(float(used_kb), size_kb)
    used_pct = min(100.0, 100.0 * used_kb / size_kb) if size_kb else 0.0
    return used_pct, 100.0 - used_pct


def _usage_title(part: dict[str, Any], used_kb: float) -> str:
    size_kb = float(part["size_kb"])
    used_kb = min(float(used_kb), size_kb)
    free_kb = max(size_kb - used_kb, 0)
    label = part["label"]
    if free_kb <= 0:
        return f"{label}: ({kb_label(used_kb)} used)"
    _, free_pct = _used_free_pcts(used_kb, size_kb)
    return f"{label}: ({kb_label(used_kb)} used, {kb_label(free_kb)} free ({free_pct:.0f}%))"


def _tooltip_align_class(segment_start_pct: float, width_pct: float) -> str:
    center_pct = segment_start_pct + width_pct / 2.0
    if center_pct < TOOLTIP_ALIGN_START_THRESHOLD_PCT:
        return " memory-viz-segment-tooltip-start"
    return ""


def _render_segment(
    part: dict[str, Any],
    total_kb: float,
    threshold_kb: float,
    segment_start_pct: float,
) -> tuple[str, float]:
    if part.get("legend_only"):
        return "", 0.0

    used_color, free_color = _partition_colors(part["id"])
    size_kb = float(part["size_kb"])
    width_pct = 100.0 * size_kb / total_kb if total_kb else 0.0
    min_width_pct = 100.0 * threshold_kb / total_kb if total_kb else 0.0
    if part["id"] not in ALWAYS_VISIBLE_BAR_IDS and width_pct < min_width_pct:
        return "", 0.0

    used_kb = part.get("used_kb")
    title = f'{part["label"]}: {kb_label(size_kb)}'
    inner = (
        f'<span class="memory-viz-fill memory-viz-fill-used" '
        f'style="width:100%;background:{used_color}"></span>'
    )

    if used_kb is not None and part["id"] in SPLIT_USAGE_IDS:
        used_kb = float(used_kb)
        title = _usage_title(part, used_kb)
        used_pct, free_pct = _used_free_pcts(used_kb, size_kb)
        inner = (
            f'<span class="memory-viz-fill memory-viz-fill-used" '
            f'style="width:{used_pct:.2f}%;background:{used_color}"></span>'
            f'<span class="memory-viz-fill memory-viz-fill-free" '
            f'style="width:{free_pct:.2f}%;background:{free_color}"></span>'
        )

    tooltip_class = _tooltip_align_class(segment_start_pct, width_pct)
    return (
        f'<div class="memory-viz-segment memory-viz-segment-{part["id"]}{tooltip_class}" '
        f'style="flex:0 0 {width_pct:.4f}%" '
        f'data-tooltip="{html.escape(title, quote=True)}">'
        f"{inner}</div>",
        width_pct,
    )


def _render_bar(
    partitions: list[dict[str, Any]],
    total_kb: float,
    threshold_kb: float,
    aria_label: str,
) -> str:
    segments = []
    segment_start_pct = 0.0
    for part in partitions:
        segment, width_pct = _render_segment(part, total_kb, threshold_kb, segment_start_pct)
        if segment:
            segments.append(segment)
            segment_start_pct += width_pct
    return (
        f'<div class="memory-viz-bar" role="img" '
        f'aria-label="{html.escape(aria_label, quote=True)}">'
        f'{"".join(segments)}</div>'
    )


def _legend_swatch(color: str) -> str:
    return f'<span class="memory-viz-swatch" style="background:{color}"></span>'


def _legend_item(label: str, color: str) -> str:
    return (
        f'<span class="memory-viz-legend-item">{_legend_swatch(color)}{html.escape(label)}</span>'
    )


def _legend_items_used_free(part_id: str, label_base: str) -> list[str]:
    used_color, free_color = _partition_colors(part_id)
    return [
        _legend_item(f"{label_base} ({suffix})", color)
        for suffix, color in (("used", used_color), ("free", free_color))
    ]


def _collect_legend_items(
    nvm_partitions: list[dict[str, Any]],
    order: tuple[str, ...],
) -> list[str]:
    parts = {part["id"]: part for part in nvm_partitions}
    order_index = {part_id: index for index, part_id in enumerate(order)}
    sorted_ids = sorted(
        parts,
        key=lambda part_id: (order_index.get(part_id, len(order)), part_id),
    )

    items: list[str] = []
    for part_id in sorted_ids:
        part = parts[part_id]
        if part_id in SPLIT_USAGE_IDS:
            items.extend(_legend_items_used_free(part_id, part["label"]))
        else:
            items.append(_legend_item(part["label"], _partition_colors(part_id)[0]))
    return items


def _collect_board_legend_items(
    samples: list[dict[str, Any]],
    partition_attr: str,
    order: tuple[str, ...],
) -> list[str]:
    parts = collect_board_partitions(samples, partition_attr)
    if not parts:
        return []
    return _collect_legend_items(list(parts.values()), order)


def _render_row_label(text: str) -> str:
    return f'<div class="memory-viz-row-label">{html.escape(text)}</div>'


def _render_legend_div(css_class: str, items: list[str]) -> str:
    if not items:
        return ""
    return f'<div class="memory-viz-legend {css_class}">{"".join(items)}</div>'


def _render_bar_stack(
    row_label: str,
    bar_html: str,
    *,
    stack_class: str = "",
    bar_wrap_class: str = "memory-viz-bar-wrap",
    width_pct: float | None = None,
) -> str:
    width_style = f' style="width:{width_pct:.4f}%"' if width_pct is not None else ""
    stack_attrs = f' class="memory-viz-bar-stack{stack_class}"{width_style}'
    return (
        f"<div{stack_attrs}>"
        f"{_render_row_label(row_label)}"
        f'<div class="{bar_wrap_class}">{bar_html}</div>'
        f"</div>"
    )


def _nvm_width_pct(part_kb: float, reference_kb: float) -> float:
    if reference_kb <= 0:
        return 100.0
    return min(100.0, 100.0 * part_kb / reference_kb)


def _grid_style(nvm_total_kb: float, ram_total_kb: float) -> str:
    return f"--memory-viz-nvm-fr:{int(nvm_total_kb)}fr;--memory-viz-ram-fr:{int(ram_total_kb)}fr"


def _sample_plain_label(sample: dict[str, Any]) -> str:
    return plain_tab_label(sample_title(sample))


def _render_board_header(
    board: dict[str, Any],
    samples: list[dict[str, Any]],
) -> str:
    nvm_total_kb = float(board["nvm_total_kb"])
    ram_total_kb = float(board["ram_total_kb"])
    external_total_kb = board_external_total_kb(samples)

    nvm_legend = _render_legend_div(
        "memory-viz-legend-nvm",
        _collect_board_legend_items(samples, "nvm_partitions", NVM_LEGEND_ORDER),
    )

    external_title = ""
    external_legend = ""
    if external_total_kb is not None:
        ext_width = _nvm_width_pct(external_total_kb, nvm_total_kb)
        external_title = (
            f'<div class="memory-viz-panel-title memory-viz-panel-title-secondary" '
            f'style="width:{ext_width:.4f}%">'
            f"{region_title('external', external_total_kb)}</div>"
        )
        external_legend = _render_legend_div(
            "memory-viz-legend-external",
            _collect_board_legend_items(
                samples,
                "external_nvm_partitions",
                EXTERNAL_NVM_LEGEND_ORDER,
            ),
        )

    ram_legend = _render_legend_div(
        "memory-viz-legend-ram",
        _legend_items_used_free("ram", "RAM"),
    )

    return (
        f'<div class="memory-viz-chart-header">'
        f'<div class="memory-viz-panel-title memory-viz-panel-title-sample">Sample</div>'
        f'<div class="memory-viz-header-nvm">'
        f'<div class="memory-viz-panel-title">{region_title("nvm", nvm_total_kb)}</div>'
        f"{nvm_legend}"
        f"{external_title}"
        f"{external_legend}"
        f"</div>"
        f'<div class="memory-viz-header-ram">'
        f'<div class="memory-viz-panel-title">{region_title("ram", ram_total_kb)}</div>'
        f"{ram_legend}"
        f"</div>"
        f"</div>"
    )


def _render_sample_nvm_html(
    sample: dict[str, Any],
    reference_nvm_kb: float,
    threshold_kb: float,
) -> str:
    title = _sample_plain_label(sample)
    internal_bar = _render_bar(
        sample.get("nvm_partitions", []),
        reference_nvm_kb,
        threshold_kb,
        f"{title} internal NVM",
    )

    external_parts = sample.get("external_nvm_partitions") or []
    external_total_kb = sample.get("external_nvm_total_kb")
    external_stack = ""
    if external_parts and external_total_kb:
        ext_total_kb = float(external_total_kb)
        external_bar = _render_bar(
            external_parts,
            ext_total_kb,
            threshold_kb,
            f"{title} external NVM",
        )
        external_stack = _render_bar_stack(
            "External NVM",
            external_bar,
            stack_class=" memory-viz-bar-stack-external",
            bar_wrap_class="memory-viz-bar-wrap memory-viz-bar-wrap-external",
            width_pct=_nvm_width_pct(ext_total_kb, reference_nvm_kb),
        )

    return (
        f'<div class="memory-viz-nvm-column">'
        f'{_render_bar_stack("Internal NVM", internal_bar)}'
        f"{external_stack}</div>"
    )


def _render_sample_ram_html(
    sample: dict[str, Any],
    board_ram_total_kb: float,
    threshold_kb: float,
) -> str:
    ram_total_kb = float(board_ram_total_kb)
    ram_used_kb = min(float(sample["ram_used_kb"]), ram_total_kb)
    ram_bar = _render_bar(
        [
            {
                "id": "ram",
                "label": "RAM",
                "size_kb": ram_total_kb,
                "used_kb": ram_used_kb,
            }
        ],
        ram_total_kb,
        threshold_kb,
        f'{_sample_plain_label(sample)} RAM',
    )
    return f'<div class="memory-viz-ram-column">{_render_bar_stack("RAM", ram_bar)}</div>'


def _build_board_nodes(
    directive: SphinxDirective,
    board: dict[str, Any],
    threshold_kb: float,
) -> nodes.Element:
    samples = board.get("samples", [])
    nvm_total_kb = float(board["nvm_total_kb"])
    ram_total_kb = float(board["ram_total_kb"])

    wrapper = nodes.container()
    wrapper["classes"] = ["memory-viz-board-outer"]
    wrapper += nodes.raw(
        "",
        f'<div class="memory-viz-board" data-board="{html.escape(board["board_id"], quote=True)}" '
        f'style="{_grid_style(nvm_total_kb, ram_total_kb)}">',
        format="html",
    )

    board_node = nodes.container()
    board_node["classes"] = ["memory-viz-board-inner"]
    board_node += nodes.raw("", _render_board_header(board, samples), format="html")

    samples_node = nodes.container()
    samples_node["classes"] = ["memory-viz-samples"]

    for sample in samples:
        row = nodes.container()
        row["classes"] = ["memory-viz-sample-row"]
        row += parse_rst_label(
            directive,
            sample_title(sample),
            css_class="memory-viz-sample-label",
            source_name="<memory_viz>",
        )
        row += nodes.raw(
            "",
            _render_sample_nvm_html(sample, nvm_total_kb, threshold_kb),
            format="html",
        )
        row += nodes.raw(
            "",
            _render_sample_ram_html(sample, ram_total_kb, threshold_kb),
            format="html",
        )
        samples_node += row

    board_node += samples_node
    wrapper += board_node
    wrapper += nodes.raw("", "</div>", format="html")
    return wrapper


class MemoryUsageBoard(SphinxDirective):
    """Render one board chart (used inside generated board tabs)."""

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        "board-id": directives.unchanged_required,
    }

    def run(self) -> list[nodes.Node]:
        boards = getattr(self.env, ENV_BOARD_KEY, {})
        entry = boards.get(self.options["board-id"])
        if entry is None:
            return []
        board, threshold_kb = entry
        return [_build_board_nodes(self, board, threshold_kb)]


class MemoryUsage(SphinxDirective):
    """Render sample memory usage bar charts from a YAML data file."""

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True
    has_content = False
    option_spec = {
        "file": directives.unchanged_required,
    }

    def run(self) -> list[nodes.Node]:
        data = load_memory_yaml(self, self.options["file"])
        threshold_kb = legend_threshold_kb(data)
        return build_board_tabs(
            self,
            sort_boards_by_internal_memory(data.get("boards", [])),
            lambda board: subdirective_tab_lines(board, "memory-usage-board"),
            source_name="<memory_usage>",
            env_attr=ENV_BOARD_KEY,
            env_value=lambda board: (board, threshold_kb),
        )


def setup(app: Sphinx) -> dict[str, Any]:
    return extension_setup(
        app,
        version=__version__,
        css_filename="memory_viz.css",
        directives={
            "memory-usage": MemoryUsage,
            "memory-usage-board": MemoryUsageBoard,
        },
    )

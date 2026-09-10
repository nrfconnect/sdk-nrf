"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

from dataclasses import dataclass, field
from typing import Iterable, cast, override

import utils
import yaml
from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective
from sphinx.util.typing import ExtensionMetadata

__version__ = "0.0.1"

_RELEASE_PATH = utils.get_projdir("nrf") / "release.yaml"


@dataclass
class _SoCInfo:
    soc: str
    status: str
    release: str = field(default="")


class SoCMaturityTable(SphinxDirective):
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    has_content = False

    _ordering = ["active", "pre-release", "maintenance", "community"]

    @override
    def run(self) -> list[nodes.Node]:
        table = nodes.table()
        tgroup = nodes.tgroup(cols=3)
        table += tgroup

        tgroup += nodes.colspec(colwidth=1)
        tgroup += nodes.colspec(colwidth=1)
        tgroup += nodes.colspec(colwidth=1)

        thead = nodes.thead()
        tgroup += thead
        thead += self._render_header()

        tbody = nodes.tbody()
        tgroup += tbody
        for info in self._socs():
            tbody += self._render_row(info)

        return [table]

    def _render_header(self) -> nodes.Node:
        row = nodes.row()

        soc_entry = nodes.entry()
        soc_entry += nodes.paragraph(text="SoC")

        status_entry = nodes.entry()
        status_entry += nodes.paragraph(text="Status")

        release_entry = nodes.entry()
        release_entry += nodes.paragraph(text="Release")

        row += soc_entry
        row += status_entry
        row += release_entry
        return row

    def _render_row(self, info: _SoCInfo) -> nodes.Node:
        row = nodes.row()

        soc_entry = nodes.entry()
        soc_entry += nodes.paragraph(text=info.soc)

        status_entry = nodes.entry()
        status_entry += nodes.paragraph(text=info.status)

        release_entry = nodes.entry()
        release_entry += nodes.paragraph(text=info.release)

        row += soc_entry
        row += status_entry
        row += release_entry
        return row

    def _socs(self) -> Iterable[_SoCInfo]:
        info = yaml.safe_load(_RELEASE_PATH.read_text())
        return sorted(
            [
                _SoCInfo(elem["soc"], elem["status"], elem.get("release", ""))
                for elem in info.get("devices", [])
            ],
            key=lambda i: self._ordering.index(i.status),
        )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("soc-maturity-table", SoCMaturityTable)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

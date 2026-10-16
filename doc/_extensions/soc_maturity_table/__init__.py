"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata

from .rendering import SoCMaturityTable

__version__ = "0.0.1"

RESOURCES_DIR = Path(__file__).parent / "static"

def _install(app: Sphinx) -> None:
    app.config.html_static_path.append(str(RESOURCES_DIR))

def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("soc-maturity-table", SoCMaturityTable)
    app.add_css_file("soc_maturity_table.css")
    app.connect("builder-inited", _install)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

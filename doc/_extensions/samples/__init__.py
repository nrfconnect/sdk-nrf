"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

from typing import Any

from sphinx.application import Sphinx

from .rendering import NCSSampleDirective, add_vscode_image

__version__ = "0.0.1"


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive('ncs-sample', NCSSampleDirective)
    app.connect('config-inited', add_vscode_image)
    app.add_css_file("sample.css")

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }

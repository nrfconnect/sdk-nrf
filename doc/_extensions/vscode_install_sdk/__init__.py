"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

from pathlib import Path
from typing import Any

from sphinx.application import Sphinx
from sphinx.config import Config
from vscode_button import add_button_css

from .rendering import VSCodeSDKInstallDirective

__version__ = "0.0.1"

INSTALL_CSS_PATH = Path(__file__).parent / "static"


def add_install_css(app: Sphinx, _: Config) -> None:
    app.config.html_static_path.append(INSTALL_CSS_PATH.as_posix())


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_directive('ncs-install-vscode', VSCodeSDKInstallDirective)
    app.connect('config-inited', add_button_css)
    app.connect('config-inited', add_install_css)
    app.add_css_file("install.css")

    return {
        'version': __version__,
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }

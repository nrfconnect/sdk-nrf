#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfigdiff extension for displaying changes in kconfig
files between releases.
"""

from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata

from .kconfig_utils import RESOURCES_DIR
from .rendering import KconfigDiffDirective

__version__ = "0.1.0"


def kconfigdiff_install(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("kconfigdiff", KconfigDiffDirective)
    app.connect("builder-inited", kconfigdiff_install)
    app.add_css_file("kconfigdiff.css")

    app.add_config_value("kconfigdiff_should_build", False, "env", types=bool)

    return {
        "version": __version__,
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

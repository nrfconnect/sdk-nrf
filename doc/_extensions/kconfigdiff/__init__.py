#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfigdiff extension for displaying changes in kconfig
files between releases.
"""

import logging
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata
from versions import Versions, get_versions

from .kconfig_utils import RESOURCES_DIR
from .legend import KconfigDiffLegendDirective
from .rendering import KconfigDiffDirective

VERSIONS_FILE = Path(__file__).parents[2] / "versions.json"

logger = logging.Logger(__name__)

__version__ = "0.1.0"


def get_kconfig_versions(app: Sphinx) -> tuple[str, str] | None:
    current = "latest"

    versions = get_versions(app).normalized().patchlevel()
    if not versions:
        logger.error("Ill formatted versions file")
        return None

    minor_versions = versions.minor()
    if app.config.kconfigdiff_is_release:
        current = versions.latest()
        if len(minor_versions) >= 2 and Versions.is_minor(current):
            return current, minor_versions.all()[1]
        elif len(versions) >= 2:
            return current, versions.all()[1]
        else:
            logger.error("Not enough versions to generate comparison")
            return None

    if prev := minor_versions.latest():
        return current, prev

    logger.error("Not enough versions to generate comparison")
    return None


def kconfigdiff_install(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())

    versions = get_kconfig_versions(app)
    app.config.kconfigdiff_versions = versions

    if versions:
        latest, prev = versions

        app.config.rst_prolog = (
            (app.config.rst_prolog or "")
            + "\n"
            + (
                f".. |kconfigdiff_current| replace:: **nRF Connect SDK {latest}**\n"
                f".. |kconfigdiff_previous| replace:: **nRF Connect SDK {prev}**\n"
            )
        )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("kconfigdiff", KconfigDiffDirective)
    app.add_directive("kconfigdiff-legend", KconfigDiffLegendDirective)
    app.connect("builder-inited", kconfigdiff_install)
    app.add_css_file("kconfigdiff.css")

    app.add_config_value("kconfigdiff_should_build", False, "env", types=bool)
    app.add_config_value("kconfigdiff_is_release", False, "env", types=bool)

    return {
        "version": __version__,
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

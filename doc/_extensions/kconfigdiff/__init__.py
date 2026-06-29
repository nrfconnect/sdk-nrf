#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Kconfigdiff extension for displaying changes in kconfig
files between releases.
"""

import json
import logging
import re
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata

from .kconfig_utils import RESOURCES_DIR
from .rendering import KconfigDiffDirective

VERSIONS_FILE = Path(__file__).parents[2] / "versions.json"

logger = logging.Logger(__name__)

__version__ = "0.1.0"

VERSION_REGEX = re.compile(r"^\d+\.\d+\.\d+$")
MAJOR_VERSION_REGEX = re.compile(r"^\d+\.\d+\.0$")


def is_major(version: str):
    return MAJOR_VERSION_REGEX.match(version)


def get_major_version(versions: list[str]) -> str | None:
    return next((v for v in versions if is_major(v)), None)


def get_versions(app) -> tuple[str, str] | None:
    current = "latest"

    with open(VERSIONS_FILE, "rb") as f:
        versions = json.load(f)
        if not versions:
            logger.error("Ill formatted versions file")
            return None

        if versions[0].endswith("99"):
            # skip the placeholder .99 version
            versions.pop(0)

        # Filter out preview versions (and other -addition versions)
        versions = [v for v in versions if VERSION_REGEX.match(v)]

        if app.config.kconfigdiff_is_release:
            current = versions[0]
            if is_major(current) and (prev := get_major_version(versions[1:])):
                return current, prev
            elif len(versions) >= 2:
                return current, versions[1]
            else:
                logger.error("Not enough versions to generate comparison")
                return None

        if prev := get_major_version(versions):
            return current, prev

        logger.error("Not enough versions to generate comparison")
        return None


def kconfigdiff_install(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())

    versions = get_versions(app)
    app.config.kconfigdiff_versions = versions

    if versions:
        latest, prev = versions

        app.config.rst_prolog = (
            (app.config.rst_prolog or "")
            + "\n"
            + (
                f".. |kconfigdiff_current| replace:: {latest}\n"
                f".. |kconfigdiff_previous| replace:: {prev}\n"
            )
        )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("kconfigdiff", KconfigDiffDirective)
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

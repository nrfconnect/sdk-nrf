"""
Kconfdiff extension for displaying changes in kconfig
files between releases.
"""

import json
import logging
import re
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata

from .kconf_utils import RESOURCES_DIR
from .rendering import KconfDiffDirective

VERSIONS_FILE = Path(__file__).parents[2] / "versions.json"

logger = logging.Logger(__name__)

__version__ = "0.1.0"


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
        versions = [v for v in versions if re.match(r"^\d+\.\d+\.\d+$", v)]

        if app.config.kconfdiff_is_release and len(versions) >= 2:
            current = versions[0]
            prev = versions[1]
        elif versions:
            prev = versions[0]
        else:
            logger.error("Not enough versions to generate comparison")
            return None

        return current, prev


def kconfdiff_install(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())

    versions = get_versions(app)
    app.config.kconfdiff_versions = versions

def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("kconfdiff", KconfDiffDirective)
    app.connect("builder-inited", kconfdiff_install)
    app.add_css_file("kconfdiff.css")

    app.add_config_value("kconfdiff_should_build", False, "env", types=bool)
    app.add_config_value("kconfdiff_is_release", False, "env", types=bool)

    return {
        "version": __version__,
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

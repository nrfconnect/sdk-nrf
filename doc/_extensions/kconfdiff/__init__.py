"""
Kconfdiff extension for displaying changes in kconfig
files between releases.
"""

from page_filter import read_versions
from sphinx.application import Sphinx
from sphinx.util.typing import ExtensionMetadata

from .kconf_utils import RESOURCES_DIR
from .rendering import KconfDiffDirective

__version__ = "0.1.0"


def kconfdiff_install(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())
    read_versions(app)


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_directive("kconfdiff", KconfDiffDirective)
    app.connect("builder-inited", kconfdiff_install)
    app.add_css_file("kconfdiff.css")

    app.add_config_value("kconfdiff_should_build", False, "env", types=bool)

    return {
        "version": __version__,
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

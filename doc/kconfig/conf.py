# Kconfig documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Kconfig reference"
copyright = "2019-2022, Nordic Semiconductor"
author = "Nordic Semiconductor"
# NOTE: use blank space as version to preserve space
version = "&nbsp;"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = ["zephyr.kconfig", "ncs_cache", "zephyr.external_content"]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_title = project
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("kconfig")}

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "kconfig"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "kconfig", "*.rst"),
]

# Options for zephyr.kconfig -----------------------------------------------------

kconfig_generate_db = True
kconfig_ext_paths = [ZEPHYR_BASE, NRF_BASE]

# Adding NCS_ specific entries. Can be removed when the NCSDK-14227 improvement
# task has been completed.
os.environ["NCS_MEMFAULT_FIRMWARE_SDK_KCONFIG"] = str(
    NRF_BASE
    / ".."
    / "modules"
    / "lib"
    / "memfault-firmware-sdk"
    / "ports"
    / "zephyr"
    / "Kconfig"
)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/kconfig.css")

    utils.add_google_analytics(app)

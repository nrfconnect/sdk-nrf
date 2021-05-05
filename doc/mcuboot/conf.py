# MCUboot documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

MCUBOOT_BASE = os.environ.get("MCUBOOT_BASE")
if not MCUBOOT_BASE:
    raise FileNotFoundError("MCUBOOT_BASE not defined")
MCUBOOT_BASE = Path(MCUBOOT_BASE)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# General configuration --------------------------------------------------------

project = "MCUboot"
copyright = "2019-2021"
version = release = "1.7.99"

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = ["sphinx.ext.intersphinx", "recommonmark", "external_content"]
source_suffix = [".rst", ".md"]
master_doc = "wrapper"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|zephyr)"]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("mcuboot")}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "mcuboot", "*.rst"),
    (MCUBOOT_BASE / "docs", "*.md")
]


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/mcuboot.css")

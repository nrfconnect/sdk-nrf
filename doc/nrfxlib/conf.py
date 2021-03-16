# nrfxlib documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

NRFXLIB_BUILD = os.environ.get("NRFXLIB_BUILD")
if not NRFXLIB_BUILD:
    raise FileNotFoundError("NRFXLIB_BUILD not defined")
NRFXLIB_BUILD = Path(NRFXLIB_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "utils"))
import utils

# General configuration --------------------------------------------------------

project = "nrfxlib"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "1.5.99"

sys.path.insert(0, str(NRF_BASE / "scripts" / "sphinx_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "sphinxcontrib.mscgen",
    "inventory_builder",
]
master_doc = "README"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|nrf)"]
rst_epilog = """
.. include:: /doc/links.txt
.. include:: /doc/shortcuts.txt
"""

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("nrfxlib")}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

nrf_mapping = utils.get_intersphinx_mapping("nrf")
if nrf_mapping:
    intersphinx_mapping["nrf"] = nrf_mapping

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrfxlib": str(NRFXLIB_BUILD / "doxygen" / "xml")}
breathe_default_project = "nrfxlib"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/nrfxlib.css")

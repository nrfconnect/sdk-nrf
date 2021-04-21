# nrf documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

NRF_BUILD = os.environ.get("NRF_BUILD")
if not NRF_BUILD:
    raise FileNotFoundError("NRF_BUILD not defined")
NRF_BUILD = Path(NRF_BUILD)

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    raise FileNotFoundError("ZEPHYR_BASE not defined")
ZEPHYR_BASE = Path(ZEPHYR_BASE)

sys.path.insert(0, str(NRF_BASE / "doc" / "utils"))
import utils

# General configuration --------------------------------------------------------

project = "nRF Connect SDK"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "1.5.99"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "scripts" / "sphinx_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "interbreathe",
    "table_from_rows",
    "options_from_kconfig",
    "ncs_include",
    "sphinxcontrib.mscgen",
    "sphinx_tabs.tabs",
    "zephyr.html_redirects",
]

linkcheck_ignore = [
    # intersphinx links
    r"(\.\.(\\|/))+(zephyr|kconfig|nrfxlib|mcuboot)",
    # redirecting and used in release notes
    "https://github.com/nrfconnect/nrfxlib",
    # link to access local documentation
    "http://localhost:8000/latest/index.html",
    # SES download links
    r"https://(www.)?segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_.+(_x\d+)?",
    # requires login
    "https://portal.azure.com/",
    # requires login
    "https://threadgroup.atlassian.net/wiki/spaces/",
    # used as example in doxygen
    "https://google.com:443",
]

linkcheck_anchors_ignore = [r"page="]

rst_epilog = """
.. include:: /links.txt
.. include:: /shortcuts.txt
.. include:: /versions.txt
"""

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("nrf")}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

zephyr_mapping = utils.get_intersphinx_mapping("zephyr")
if zephyr_mapping:
    intersphinx_mapping["zephyr"] = zephyr_mapping

mcuboot_mapping = utils.get_intersphinx_mapping("mcuboot")
if mcuboot_mapping:
    intersphinx_mapping["mcuboot"] = mcuboot_mapping

nrfxlib_mapping = utils.get_intersphinx_mapping("nrfxlib")
if nrfxlib_mapping:
    intersphinx_mapping["nrfxlib"] = nrfxlib_mapping

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrf": str(NRF_BUILD / "doxygen" / "xml")}
breathe_default_project = "nrf"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for ncs_include ------------------------------------------------------

ncs_include_mapping = {
    "nrf": utils.get_rstdir("nrf"),
    "zephyr": utils.get_rstdir("zephyr"),
}

# Options for html_redirect ----------------------------------------------------

html_redirect_pages = [
    ("gs_ins_windows", "gs_installing"),
    ("gs_ins_linux", "gs_installing"),
    ("gs_ins_mac", "gs_installing"),
]


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/nrf.css")

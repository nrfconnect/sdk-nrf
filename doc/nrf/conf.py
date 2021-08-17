# nrf documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "nRF Connect SDK"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "1.6.99"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "interbreathe",
    "table_from_rows",
    "options_from_kconfig",
    "ncs_include",
    "sphinxcontrib.mscgen",
    "zephyr.html_redirects",
    "zephyr.warnings_filter",
    "ncs_cache",
    "external_content",
    "doxyrunner",
    "sphinx_tabs.tabs",
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
html_static_path = [str(NRF_BASE / "doc" / "_static")]
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

nrfx_mapping = utils.get_intersphinx_mapping("nrfx")
if nrfx_mapping:
    intersphinx_mapping["nrfx"] = nrfx_mapping

# -- Options for doxyrunner plugin ---------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrf" / "nrf.doxyfile.in"
doxyrunner_outdir = utils.get_builddir() / "nrf" / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRF_BASE": str(NRF_BASE),
    "NRF_BINARY_DIR": str(utils.get_builddir() / "nrf"),
}

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrf": str(doxyrunner_outdir / "xml")}
breathe_default_project = "nrf"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for ncs_include ------------------------------------------------------

ncs_include_mapping = {
    "nrf": utils.get_srcdir("nrf"),
    "nrfxlib": utils.get_srcdir("nrfxlib"),
    "zephyr": utils.get_srcdir("zephyr"),
}

# Options for html_redirect ----------------------------------------------------

html_redirect_pages = [
    ("gs_ins_windows", "gs_installing"),
    ("gs_ins_linux", "gs_installing"),
    ("gs_ins_mac", "gs_installing"),
    ("examples", "samples"),
]

# -- Options for zephyr.warnings_filter ----------------------------------------

warnings_filter_config = str(NRF_BASE / "doc" / "nrf" / "known-warnings.txt")
warnings_filter_silent = False

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "nrf", "*"),
    (NRF_BASE, "applications/**/*.rst"),
    (NRF_BASE, "applications/**/doc"),
    (NRF_BASE, "samples/**/*.rst"),
    (NRF_BASE, "scripts/**/*.rst"),
    (NRF_BASE, "tests/**/*.rst"),
]
external_content_keep = ["versions.txt"]

# Options for table_from_rows --------------------------------------------------

table_from_rows_base_dir = NRF_BASE

# Options for options_from_kconfig ---------------------------------------------

options_from_kconfig_base_dir = NRF_BASE
options_from_kconfig_zephyr_dir = ZEPHYR_BASE

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "nrf"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/nrf.css")

#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrfxlib documentation build configuration file

import os
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")
NRFXLIB_BASE = utils.get_projdir("nrfxlib")

# General configuration --------------------------------------------------------

project = "nrfxlib"
copyright = "2019-2024, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "2.7.99"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "sphinxcontrib.mscgen",
    "inventory_builder",
    "zephyr.kconfig",
    "warnings_filter",
    "zephyr.external_content",
    "zephyr.doxyrunner",
]
master_doc = "README"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|nrf)"]
rst_epilog = """
.. include:: /doc/links.txt
.. include:: /doc/shortcuts.txt
"""

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docset": "nrfxlib", "docsets": utils.ALL_DOCSETS}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

zephyr_mapping = utils.get_intersphinx_mapping("zephyr")
if zephyr_mapping:
    intersphinx_mapping["zephyr"] = zephyr_mapping

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

nrf_mapping = utils.get_intersphinx_mapping("nrf")
if nrf_mapping:
    intersphinx_mapping["nrf"] = nrf_mapping

# -- Options for warnings_filter -----------------------------------------------

warnings_filter_config = str(NRF_BASE / "doc" / "nrfxlib" / "known-warnings.txt")

# -- Options for doxyrunner plugin ---------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrfxlib" / "nrfxlib.doxyfile.in"
doxyrunner_outdir = utils.get_builddir() / "nrfxlib" / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRFXLIB_BASE": str(NRFXLIB_BASE),
    "OUTPUT_DIRECTORY": str(doxyrunner_outdir),
}

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrfxlib": str(doxyrunner_outdir / "xml")}
breathe_default_project = "nrfxlib"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for external_content -------------------------------------------------

external_content_contents = [(NRFXLIB_BASE, "**/*.rst"), (NRFXLIB_BASE, "**/doc/")]


def setup(app):
    app.add_css_file("css/nrfxlib.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

#
# Copyright (c) 2025 Nordic Semiconductor
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
copyright = "2019-2025, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "sphinxcontrib.mscgen",
    "inventory_builder",
    "warnings_filter",
    "sphinx_tabs.tabs",
    "zephyr.kconfig",
    "zephyr.external_content",
    "zephyr.doxyrunner",
    "zephyr.doxybridge",
    "zephyr.domain",
    "zephyr.gh_utils",
    "sphinx_llms_txt",
]
master_doc = "README"

linkcheck_ignore = [r"\.\.(\\|/)"]
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

# -- Options for doxyrunner plugin ---------------------------------------------

_doxyrunner_outdir = utils.get_builddir() / "html" / "nrfxlib" / "doxygen"

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_projects = {
    "nrfxlib": {
        "doxyfile": NRF_BASE / "doc" / "nrfxlib" / "nrfxlib.doxyfile.in",
        "outdir": _doxyrunner_outdir,
        "fmt": True,
        "fmt_vars": {
            "NRF_BASE": str(NRF_BASE),
            "DOCSET_SOURCE_BASE": str(NRFXLIB_BASE),
            "DOCSET_BUILD_DIR": str(_doxyrunner_outdir),
            "DOCSET_VERSION": version,
        }
    }
}

# -- Options for doxybridge plugin ---------------------------------------------

doxybridge_projects = {"nrfxlib": _doxyrunner_outdir}

# -- Options for warnings_filter -----------------------------------------------

warnings_filter_config = str(NRF_BASE / "doc" / "nrfxlib" / "warnings-inventory.txt")
warnings_filter_builders = ["inventory"]

# Options for external_content -------------------------------------------------

external_content_contents = [(NRFXLIB_BASE, "**/*.rst"), (NRFXLIB_BASE, "**/doc/")]

# -- Options for zephyr.gh_utils -----------------------------------------------

gh_link_version = "main" if version.endswith("99") else f"v{version}"
gh_link_base_url = f"https://github.com/nrfconnect/sdk-nrfxlib"


def setup(app):
    app.add_css_file("css/nrfxlib.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

# Options for sphinx-llms-txt ---------------------------------------------------

llms_txt_summary = """
# nRF Connect SDK Documentation

Version: {version}

The nRF Connect SDK is Nordic Semiconductor's software development kit for
building products based on Nordic Semiconductor SoCs.

Quick start: gsg_guides.html
Installing the nRF Connect SDK: installation/install_ncs.html
"""

# DO NOT GENERATE llms-full.txt
llms_txt_full = False
llms_txt_exclude_patterns = [
    # EXCLUDE release notes - including latest tag
    "releases_and_maturity/releases/release-notes-{}".format(
    "changelog" if version.endswith("99") else version
    ),
]

# INCLUDE source files
llms_txt_source_files = [
]

# SET base URL - including latest tag
llms_txt_html_base_url = "https://docs.nordicsemi.com/bundle/ncs-{}/page/nrf/".format(
    "latest" if version.endswith("99") else version
)

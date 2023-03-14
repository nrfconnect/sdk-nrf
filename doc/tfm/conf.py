#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# TFM documentation build configuration file

from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

TFM_BASE = utils.get_projdir("tfm")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Trusted Firmware-M"
copyright = "2017-2021, ARM CE-OSS"
author = "ARM CE-OSS"
version = "1.6.0"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "m2r2",
    "sphinx.ext.autosectionlabel",
    "sphinxcontrib.plantuml",
    "sphinx_tabs.tabs",
    "ncs_cache",
    "zephyr.external_content",
]
source_suffix = [".rst", ".md"]

exclude_patterns = ["readme.rst"]

numfig = True

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [
    str(NRF_BASE / "doc" / "_static"),
    str(TFM_BASE / "docs" / "_static"),
]
html_last_updated_fmt = None
html_show_sourcelink = True
html_show_sphinx = False
html_show_copyright = False
html_title = "Trusted Firmware-M documentation (nRF Connect SDK)"

html_theme_options = {
    "docset": "tfm",
    "docsets": utils.ALL_DOCSETS,
    "subtitle": "nRF Connect SDK",
}

# Options for autosectionlabel -------------------------------------------------

autosectionlabel_prefix_document = True
autosectionlabel_maxdepth = 2

# Options for external_content -------------------------------------------------

external_content_contents = [
    (TFM_BASE / "docs", "**/*"),
    (TFM_BASE, "platform/**/*"),
    (TFM_BASE, "tools/**/*"),
]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "tfm"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/tfm.css")

    utils.add_google_analytics(app, html_theme_options)

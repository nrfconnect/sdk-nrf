#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# TFM documentation build configuration file

import os
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
version = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "m2r2",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.intersphinx",
    "sphinxcontrib.plantuml",
    "sphinx_tabs.tabs",
    "zephyr.external_content",
]
source_suffix = [".rst", ".md"]

exclude_patterns = [
  "platform/cypress/psoc64/security/keys/readme.rst"
]

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

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = {
    "TF-M-Tests": (f"https://trustedfirmware-m.readthedocs.io/projects/tf-m-tests/en/tf-mv{version}/", None),
    "TF-M-Tools": (f"https://trustedfirmware-m.readthedocs.io/projects/tf-m-tools/en/tf-mv{version}/", None),
    "TF-M-Extras": (f"https://trustedfirmware-m.readthedocs.io/projects/tf-m-extras/en/tf-mv{version}/", None),
}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (TFM_BASE / "docs", "**/*"),
]


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/tfm.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

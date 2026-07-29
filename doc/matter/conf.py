#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import os
import sys
from pathlib import Path

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Matter"
copyright = "2020-2024, Matter Contributors"
author = "Matter Contributors"
version = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "zephyr.external_content",
]

source_suffix = [".rst"]

rst_epilog = """
.. include:: /links.txt
"""

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

nrf_mapping = utils.get_intersphinx_mapping("nrf")
if nrf_mapping:
    intersphinx_mapping["nrf"] = nrf_mapping

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False
html_title = "Matter (nRF Connect SDK)"

html_theme_options = {
    "docset": "matter",
    "docsets": utils.ALL_DOCSETS,
    "subtitle": "nRF Connect SDK",
    "collapse_navigation": False,
    "logo_url": "https://docs.nordicsemi.com"
}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "matter", "*.rst"),
    (NRF_BASE / "doc" / "matter", "*.txt"),
]


def setup(app):
    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

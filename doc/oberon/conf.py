#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Oberon PSA Crypto documentation build configuration file

import os
import sys
from pathlib import Path

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

OBERON_BASE = utils.get_projdir("oberon-psa-crypto")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Oberon PSA Crypto"
copyright = "2025, Nordic Semiconductor"
version = release = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "zephyr.kconfig",
    "sphinx.ext.intersphinx",
    "recommonmark",
    "sphinx_markdown_tables",
    "zephyr.external_content",
    "zephyr.doxyrunner",
    "zephyr.doxybridge",
]
source_suffix = [".rst", ".md"]
master_doc = "wrapper"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|zephyr)"]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False
html_title = "Oberon PSA Crypto (nRF Connect SDK)"

html_theme_options = {
    "docset": "oberon",
    "docsets": utils.ALL_DOCSETS,
    "subtitle": "nRF Connect SDK",
}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "oberon", "wrapper.rst"),
    (OBERON_BASE / "oberon" / "docs", "*.md"),
]

# -- Options for doxyrunner plugin (generates Doxygen API docs) ----------------

_doxyrunner_outdir = utils.get_builddir() / "html" / "oberon" / "doxygen"

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_projects = {
    "oberon": {
        "doxyfile": NRF_BASE / "doc" / "oberon" / "oberon.doxyfile.in",
        "outdir": _doxyrunner_outdir,
        "fmt": True,
        "fmt_vars": {
            "NRF_BASE": str(NRF_BASE),
            "DOCSET_SOURCE_BASE": str(OBERON_BASE),
            "DOCSET_BUILD_DIR": str(_doxyrunner_outdir),
            "DOCSET_VERSION": version,
        }
    }
}

# -- Options for doxybridge plugin (links Doxygen docs into Sphinx) -----------

doxybridge_projects = {"oberon": _doxyrunner_outdir}


def setup(app):
    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

# nrfxlib documentation build configuration file

import os
import re
from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

NRFXLIB_BASE = os.environ.get("NRFXLIB_BASE")
if not NRFXLIB_BASE:
    raise FileNotFoundError("NRFXLIB_BASE not defined")
NRFXLIB_BASE = Path(NRFXLIB_BASE)

NRFXLIB_BUILD = os.environ.get("NRFXLIB_BUILD")
if not NRFXLIB_BUILD:
    raise FileNotFoundError("NRFXLIB_BUILD not defined")
NRFXLIB_BUILD = Path(NRFXLIB_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# General configuration --------------------------------------------------------

project = "nrfxlib"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "1.6.99"

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "sphinxcontrib.mscgen",
    "inventory_builder",
    "external_content",
    "doxyrunner",
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

html_theme_options = {"docsets": utils.get_docsets("nrfxlib")}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

nrf_mapping = utils.get_intersphinx_mapping("nrf")
if nrf_mapping:
    intersphinx_mapping["nrf"] = nrf_mapping

# -- Options for doxyrunner plugin ---------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrfxlib" / "nrfxlib.doxyfile.in"
doxyrunner_outdir = NRFXLIB_BUILD / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRFXLIB_BASE": str(NRFXLIB_BASE),
    "OUTPUT_DIRECTORY": str(doxyrunner_outdir),
}

# create mbedtls config header (needed for Doxygen)
doxyrunner_outdir.mkdir(exist_ok=True)

fin_path = NRFXLIB_BASE / "nrf_security" / "configs" / "nrf-config.h.template"
fout_path = doxyrunner_outdir / "mbedtls_doxygen_config.h"

with open(fin_path) as fin, open(fout_path, "w") as fout:
    fout.write(
        re.sub(
            r"#cmakedefine ([A-Z0-9_-]+)",
            "\n".join(
                (
                    r"#define \1",
                    r"#define CONFIG_GLUE_\1",
                    r"#define CONFIG_CC310_\1",
                    r"#define CONFIG_VANILLA_\1",
                )
            ),
            fin.read(),
        )
    )

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrfxlib": str(doxyrunner_outdir / "xml")}
breathe_default_project = "nrfxlib"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for external_content -------------------------------------------------

external_content_contents = [(NRFXLIB_BASE, "**/*.rst"), (NRFXLIB_BASE, "**/doc/")]


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/nrfxlib.css")

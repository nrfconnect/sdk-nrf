#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrfx documentation build configuration file

from pathlib import Path
import sys
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

NRFX_BASE = utils.get_projdir("nrfx") / "nrfx"
ZEPHYR_BASE = utils.get_projdir("zephyr")

# pylint: disable=undefined-variable

# General ----------------------------------------------------------------------

# Import nrfx configuration, override as needed later
conf = eval_config_file(str(NRFX_BASE / "doc" / "sphinx" / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))
extensions.extend(["zephyr.external_content", "zephyr.doxyrunner"])

# Options for HTML output ------------------------------------------------------

html_static_path.append(str(NRF_BASE / "doc" / "_static"))
html_theme_options = {"docset": "nrfx", "docsets": utils.ALL_DOCSETS}

# -- Options for doxyrunner ----------------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrfx" / "nrfx.doxyfile.in"
doxyrunner_outdir = utils.get_builddir() / "nrfx" / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRFX_BASE": str(NRFX_BASE),
    "OUTPUT_DIRECTORY": str(doxyrunner_outdir),
}

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrfx": str(doxyrunner_outdir / "xml")}

# Options for external_content -------------------------------------------------

from zephyr.external_content import DEFAULT_DIRECTIVES

directives = DEFAULT_DIRECTIVES + ("mdinclude",)

external_content_directives = directives
external_content_contents = [
    (NRFX_BASE / "doc" / "sphinx", "**/*.rst"),
]

# pylint: enable=undefined-variable


def setup(app):
    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

# nrfx documentation build configuration file

from pathlib import Path
import sys
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parent / ".." / ".."

NRFX_BASE = os.environ.get("NRFX_BASE")
if not NRFX_BASE:
    raise FileNotFoundError("NRFX_BASE not defined")
NRFX_BASE = Path(NRFX_BASE)

NRFX_BUILD = os.environ.get("NRFX_BUILD")
if not NRFX_BUILD:
    raise FileNotFoundError("NRFX_BUILD not defined")
NRFX_BUILD = Path(NRFX_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# pylint: disable=undefined-variable

# General ----------------------------------------------------------------------

# Import nrfx configuration, override as needed later
conf = eval_config_file(str(NRFX_BASE / "doc" / "sphinx" / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))
extensions.extend(["external_content", "doxyrunner"])

# Options for HTML output ------------------------------------------------------

html_static_path.append(str(NRF_BASE / "doc" / "_static"))
html_theme_options = {"docsets": utils.get_docsets("nrfx")}

# -- Options for doxyrunner ----------------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrfx" / "nrfx.doxyfile.in"
doxyrunner_outdir = NRFX_BUILD / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRFX_BASE": str(NRFX_BASE),
    "OUTPUT_DIRECTORY": str(doxyrunner_outdir),
}

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrfx": str(doxyrunner_outdir / "xml")}

# Options for external_content -------------------------------------------------

from external_content import DEFAULT_DIRECTIVES
directives = DEFAULT_DIRECTIVES + ("mdinclude", )

external_content_directives = directives
external_content_contents = [
    (NRFX_BASE / "doc" / "sphinx", "**/*.rst"),
]

# pylint: enable=undefined-variable


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/nrfx.css")

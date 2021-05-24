# TFM documentation build configuration file

import os
from pathlib import Path
import re
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

TFM_BASE = os.environ.get("TFM_BASE")
if not TFM_BASE:
    raise FileNotFoundError("TFM_BASE not defined")
TFM_BASE = Path(TFM_BASE)

TFM_BUILD = os.environ.get("TFM_BUILD")
if not TFM_BUILD:
    raise FileNotFoundError("TFM_BUILD not defined")
TFM_BUILD = Path(TFM_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# General configuration --------------------------------------------------------

project = "Trusted Firmware-M"
copyright = "2017-2019, ARM CE-OSS"
author = "ARM CE-OSS"

with open(TFM_BASE / "CMakeLists.txt") as f:
    m = re.findall(r"TFM_VERSION ([0-9\.]+)", f.read())
    version = m[0] if m else "Unknown"

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "m2r2",
    "sphinx.ext.autosectionlabel",
    "sphinxcontrib.plantuml",
    "ncs_cache",
    "external_content",
]
source_suffix = [".rst", ".md"]

exclude_patterns = ["readme.rst"]

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

html_theme_options = {"docsets": utils.get_docsets("tfm")}

# Options for autosectionlabel -------------------------------------------------

autosectionlabel_prefix_document = True
autosectionlabel_maxdepth = 2

# Options for external_content -------------------------------------------------

external_content_contents = [
    (TFM_BASE / "docs", "index.rst"),
    (TFM_BASE, "docs/**/*"),
    (TFM_BASE, "platform/**/*"),
    (TFM_BASE, "tools/**/*"),
]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "tfm"
ncs_cache_build_dir = TFM_BUILD / ".."
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/tfm.css")

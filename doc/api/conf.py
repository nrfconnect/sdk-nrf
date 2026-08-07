import os
import sys
from pathlib import Path

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "API Reference"
copyright = "2019-2026, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "zephyr.external_content",
    "notfound.extension",
    "sphinx_copybutton",
]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docset": "api", "docsets": utils.ALL_DOCSETS}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "api", "*"),
]


def setup(app):
    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

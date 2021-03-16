# Kconfig documentation build configuration file

from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parent / ".." / ".."

sys.path.insert(0, str(NRF_BASE / "doc" / "utils"))
import utils

# General configuration --------------------------------------------------------

project = "Kconfig reference"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
# NOTE: use blank space as version to preserve space
version = "&nbsp;"

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "static")]
html_title = project
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("kconfig")}


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/kconfig.css")

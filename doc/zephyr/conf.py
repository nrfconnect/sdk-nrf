# Zephyr documentation build configuration file

from pathlib import Path
import sys
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parent / ".." / ".."

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    raise FileNotFoundError("ZEPHYR_BASE not defined")
ZEPHYR_BASE = Path(ZEPHYR_BASE)

sys.path.insert(0, str(NRF_BASE / "doc" / "utils"))
import utils

# pylint: disable=undefined-variable

# General ----------------------------------------------------------------------

# Import all Zephyr configuration, override as needed later
conf = eval_config_file(str(ZEPHYR_BASE / "doc" / "conf.py"), tags)
locals().update(conf)

extensions.append("sphinx.ext.intersphinx")

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_theme_path = []
html_favicon = None
html_static_path.append(str(NRF_BASE / "doc" / "static"))
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_logo = None

html_context = {
    "show_license": True,
    "docs_title": docs_title,
    "is_release": is_release,
}

html_theme_options = {"docsets": utils.get_docsets("zephyr")}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# pylint: enable=undefined-variable


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/zephyr.css")

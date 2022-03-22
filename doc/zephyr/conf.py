# Zephyr documentation build configuration file

import os
from pathlib import Path
import sys
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")

# pylint: disable=undefined-variable,used-before-assignment

# General ----------------------------------------------------------------------

# Import all Zephyr configuration, override as needed later
os.environ["ZEPHYR_BASE"] = str(ZEPHYR_BASE)
os.environ["ZEPHYR_BUILD"] = str(utils.get_builddir() / "zephyr")

conf = eval_config_file(str(ZEPHYR_BASE / "doc" / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "ncs_cache",
] + extensions

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_theme_path = []
html_favicon = None
html_static_path.append(str(NRF_BASE / "doc" / "_static"))
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

# -- Options for zephyr.warnings_filter ----------------------------------------

warnings_filter_silent = True

# -- Options for zephyr.kconfig ------------------------------------------------

kconfig_generate_db = False

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "zephyr"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"

# pylint: enable=undefined-variable,used-before-assignment


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/zephyr.css")

    utils.add_google_analytics(app)

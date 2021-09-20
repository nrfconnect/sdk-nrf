# Apple Find My documentation build configuration file

import os
import re
from pathlib import Path
import sys

# Utility function -------------------------------------------------------------

def find_my_version(releases_dir):
    files = os.listdir(releases_dir)
    versions_str = [re.match(r".*-(.*)\.", x)[1]
                    for x in files if re.match(r"release-notes-.*\.rst$", x)]
    if "latest" in versions_str:
        return "latest"
    versions_num = [tuple(int(y) for y in x.split('.')) for x in versions_str]
    versions_num.sort(reverse=True)
    return ".".join(str(x) for x in versions_num[0])

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")
FIND_MY_BASE = utils.get_projdir("find-my")

# General configuration --------------------------------------------------------

project = "Apple Find My"
copyright = "2019-2021, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = find_my_version(FIND_MY_BASE / "doc" / "releases")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "sphinxcontrib.mscgen",
    "inventory_builder",
    "zephyr.kconfig-role",
    "ncs_cache",
    "zephyr.external_content",
    "zephyr.doxyrunner",
]
master_doc = "index"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|nrf)"]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docsets": utils.get_docsets("find-my")}

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
doxyrunner_doxyfile = NRF_BASE / "doc" / "find-my" / "find-my.doxyfile.in"
doxyrunner_outdir = utils.get_builddir() / "find-my" / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "FIND_MY_BASE": str(FIND_MY_BASE),
    "OUTPUT_DIRECTORY": str(doxyrunner_outdir),
}

# Options for breathe ----------------------------------------------------------

breathe_projects = {"find-my": str(doxyrunner_outdir / "xml")}
breathe_default_project = "find-my"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for external_content -------------------------------------------------

external_content_contents = [(FIND_MY_BASE, "**/*.rst"), (FIND_MY_BASE, "**/doc/")]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "find-my"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/find-my.css")

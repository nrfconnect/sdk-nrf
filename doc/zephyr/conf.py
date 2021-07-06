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

ZEPHYR_BUILD = os.environ.get("ZEPHYR_BUILD")
if not ZEPHYR_BUILD:
    raise FileNotFoundError("ZEPHYR_BUILD not defined")
ZEPHYR_BUILD = Path(ZEPHYR_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# pylint: disable=undefined-variable,used-before-assignment

# General ----------------------------------------------------------------------

# Import all Zephyr configuration, override as needed later
conf = eval_config_file(str(ZEPHYR_BASE / "doc" / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions.remove("zephyr.doxyrunner")
extensions = [
    "sphinx.ext.intersphinx",
    "ncs_cache",
    "external_content",
    "doxyrunner",
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

# Options for external_content -------------------------------------------------

external_content_contents = [
    (ZEPHYR_BASE / "doc", "[!_]*"),
    (ZEPHYR_BASE, "boards/**/*.rst"),
    (ZEPHYR_BASE, "boards/**/doc"),
    (ZEPHYR_BASE, "samples/**/*.rst"),
    (ZEPHYR_BASE, "samples/**/doc"),
]
external_content_keep = [
    "reference/devicetree/bindings.rst",
    "reference/devicetree/bindings/**/*",
    "reference/devicetree/compatibles/**/*",
]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "zephyr"
ncs_cache_build_dir = ZEPHYR_BUILD / ".."
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"

# pylint: enable=undefined-variable,used-before-assignment


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/zephyr.css")

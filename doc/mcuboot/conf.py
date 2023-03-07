# MCUboot documentation build configuration file

from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

MCUBOOT_BASE = utils.get_projdir("mcuboot")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "MCUboot"
copyright = "2019-2023, Nordic Semiconductor"
version = release = "1.9.99"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "zephyr.kconfig",
    "sphinx.ext.intersphinx",
    "recommonmark",
    "ncs_cache",
    "zephyr.external_content"
]
source_suffix = [".rst", ".md"]
master_doc = "wrapper"

linkcheck_ignore = [r"(\.\.(\\|/))+(kconfig|zephyr)"]

rst_epilog = """
.. include:: /links.txt
.. include:: /shortcuts.txt
"""

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False

html_theme_options = {"docset": "mcuboot", "docsets": utils.ALL_DOCSETS}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "mcuboot", "*.rst"),
    (NRF_BASE / "doc" / "mcuboot", "*.txt"),
    (MCUBOOT_BASE / "docs", "release-notes.md"),
    (MCUBOOT_BASE / "docs", "design.md"),
    (MCUBOOT_BASE / "docs", "encrypted_images.md"),
    (MCUBOOT_BASE / "docs", "imgtool.md"),
    (MCUBOOT_BASE / "docs", "ecdsa.md"),
    (MCUBOOT_BASE / "docs", "readme-zephyr.md"),
    (MCUBOOT_BASE / "docs", "testplan-mynewt.md"),
    (MCUBOOT_BASE / "docs", "testplan-zephyr.md"),
    (MCUBOOT_BASE / "docs", "release.md"),
    (MCUBOOT_BASE / "docs", "SECURITY.md"),
    (MCUBOOT_BASE / "docs", "signed_images.md"),
    (MCUBOOT_BASE / "docs", "SubmittingPatches.md"),
]

# Options for ncs_cache --------------------------------------------------------

ncs_cache_docset = "mcuboot"
ncs_cache_build_dir = utils.get_builddir()
ncs_cache_config = NRF_BASE / "doc" / "cache.yml"
ncs_cache_manifest = NRF_BASE / "west.yml"


def setup(app):
    utils.add_google_analytics(app, html_theme_options)

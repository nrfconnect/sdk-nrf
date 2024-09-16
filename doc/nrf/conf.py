#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrf documentation build configuration file

import os
from pathlib import Path
import sys
import re


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import redirects
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")
MCUBOOT_BASE = utils.get_projdir("mcuboot")

# General configuration --------------------------------------------------------

project = "nRF Connect SDK"
copyright = "2019-2024, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = "2.7.99"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "breathe",
    "interbreathe",
    "table_from_rows",
    "options_from_kconfig",
    "ncs_include",
    "manifest_revisions_table",
    "sphinxcontrib.mscgen",
    "zephyr.html_redirects",
    "warnings_filter",
    "zephyr.kconfig",
    "zephyr.external_content",
    "zephyr.doxyrunner",
    "zephyr.link-roles",
    "zephyr.domain",
    "sphinx_tabs.tabs",
    "software_maturity_table",
    "sphinx_togglebutton",
    "sphinx_copybutton",
    "notfound.extension",
    "ncs_tool_versions",
    "page_filter",
]

linkcheck_ignore = [
    # intersphinx links
    r"(\.\.(\\|/))+(zephyr|kconfig|nrfxlib|mcuboot)",
    # redirecting and used in release notes
    "https://github.com/nrfconnect/nrfxlib",
    # link to access local documentation
    "http://localhost:8000/latest/index.html",
    # SES download links
    r"https://(www.)?segger.com/downloads/embedded-studio/embeddedstudio_arm_nordic_.+(_x\d+)?",
    # requires login
    "https://portal.azure.com/",
    # requires login
    "https://threadgroup.atlassian.net/wiki/spaces/",
    # used as example in doxygen
    "https://google.com:443",
]

linkcheck_anchors_ignore = [r"page="]

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

html_theme_options = {"docset": "nrf", "docsets": utils.ALL_DOCSETS}

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

zephyr_mapping = utils.get_intersphinx_mapping("zephyr")
if zephyr_mapping:
    intersphinx_mapping["zephyr"] = zephyr_mapping

mcuboot_mapping = utils.get_intersphinx_mapping("mcuboot")
if mcuboot_mapping:
    intersphinx_mapping["mcuboot"] = mcuboot_mapping

nrfxlib_mapping = utils.get_intersphinx_mapping("nrfxlib")
if nrfxlib_mapping:
    intersphinx_mapping["nrfxlib"] = nrfxlib_mapping

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

nrfx_mapping = utils.get_intersphinx_mapping("nrfx")
if nrfx_mapping:
    intersphinx_mapping["nrfx"] = nrfx_mapping

matter_mapping = utils.get_intersphinx_mapping("matter")
if matter_mapping:
    intersphinx_mapping["matter"] = matter_mapping

tfm_mapping = utils.get_intersphinx_mapping("tfm")
if tfm_mapping:
    intersphinx_mapping["tfm"] = tfm_mapping

# -- Options for doxyrunner plugin ---------------------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = NRF_BASE / "doc" / "nrf" / "nrf.doxyfile.in"
doxyrunner_outdir = utils.get_builddir() / "nrf" / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "NRF_BASE": str(NRF_BASE),
    "NRF_BINARY_DIR": str(utils.get_builddir() / "nrf"),
}

# create mbedtls config header (needed for Doxygen)
doxyrunner_outdir.mkdir(exist_ok=True)

fin_path = NRF_BASE / "subsys" / "nrf_security" / "configs" / "legacy_crypto_config.h.template"
fout_path = doxyrunner_outdir / "mbedtls_doxygen_config.h"

with open(fin_path) as fin, open(fout_path, "w") as fout:
    fout.write(
        re.sub(
            r"#cmakedefine ([A-Z0-9_-]+)",
            r"#define \1",
            fin.read()
        )
    )

# Options for breathe ----------------------------------------------------------

breathe_projects = {"nrf": str(doxyrunner_outdir / "xml")}
breathe_default_project = "nrf"
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_separate_member_pages = True

# Options for ncs_include ------------------------------------------------------

ncs_include_mapping = {
    "nrf": utils.get_srcdir("nrf"),
    "nrfxlib": utils.get_srcdir("nrfxlib"),
    "zephyr": utils.get_srcdir("zephyr"),
}

# Options for html_redirect ----------------------------------------------------

html_redirect_pages = redirects.NRF

# -- Options for warnings_filter -----------------------------------------------

warnings_filter_config = str(NRF_BASE / "doc" / "nrf" / "known-warnings.txt")

# Options for zephyr.link-roles ------------------------------------------------

link_roles_manifest_project = "nrf"
link_roles_manifest_baseurl = "https://github.com/nrfconnect/sdk-nrf"

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "nrf", "*"),
    (NRF_BASE, "applications/**/*.rst"),
    (NRF_BASE, "applications/**/doc"),
    (NRF_BASE, "samples/**/*.rst"),
    (NRF_BASE, "scripts/**/*.rst"),
    (NRF_BASE, "tests/**/*.rst"),
]
external_content_keep = ["versions.txt"]

# Options for table_from_rows --------------------------------------------------

table_from_rows_base_dir = NRF_BASE
table_from_sample_yaml_board_reference = "/includes/sample_board_rows.txt"

# Options for ncs_tool_versions ------------------------------------------------

ncs_tool_versions_host_deps = [
    NRF_BASE / "scripts" / "tools-versions-win10.yml",
    NRF_BASE / "scripts" / "tools-versions-linux.yml",
    NRF_BASE / "scripts" / "tools-versions-darwin.yml",
]
ncs_tool_versions_python_deps = [
    ZEPHYR_BASE / "scripts" / "requirements-base.txt",
    MCUBOOT_BASE / "scripts" / "requirements.txt",
    NRF_BASE / "doc" / "requirements.txt",
    NRF_BASE / "scripts" / "requirements-build.txt",
]

# Options for options_from_kconfig ---------------------------------------------

options_from_kconfig_base_dir = NRF_BASE
options_from_kconfig_zephyr_dir = ZEPHYR_BASE

# Options for manifest_revisions_table -----------------------------------------

manifest_revisions_table_manifest = NRF_BASE / "west.yml"

# Options for sphinx_notfound_page ---------------------------------------------

notfound_urls_prefix = "/nRF_Connect_SDK/doc/{}/nrf/".format(
    "latest" if version.endswith("99") else version
)

def setup(app):
    app.add_css_file("css/nrf.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

#
# Copyright (c) 2025 Nordic Semiconductor
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
copyright = "2019-2025, Nordic Semiconductor"
author = "Nordic Semiconductor"
version = release = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "sphinx.ext.intersphinx",
    "table_from_rows",
    "options_from_kconfig",
    "ncs_include",
    "manifest_revisions_table",
    "sphinxcontrib.mscgen",
    "zephyr.html_redirects",
    "zephyr.kconfig",
    "zephyr.external_content",
    "zephyr.doxyrunner",
    "zephyr.doxybridge",
    "zephyr.link-roles",
    "zephyr.dtcompatible-role",
    "zephyr.domain",
    "zephyr.gh_utils",
    "sphinx_tabs.tabs",
    "software_maturity_table",
    "sphinx_togglebutton",
    "sphinx_copybutton",
    "notfound.extension",
    "ncs_tool_versions",
    "page_filter",
    "sphinxcontrib.plantuml",
    "sphinxcontrib.programoutput",
    "sphinx_llms_txt",
]

linkcheck_ignore = [
    # relative links (intersphinx, doxygen)
    r"\.\.(\\|/)",
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

matter_mapping = utils.get_intersphinx_mapping("matter")
if matter_mapping:
    intersphinx_mapping["matter"] = matter_mapping

tfm_mapping = utils.get_intersphinx_mapping("tfm")
if tfm_mapping:
    intersphinx_mapping["tfm"] = tfm_mapping

# -- Options for doxyrunner plugin ---------------------------------------------

_doxyrunner_outdir = utils.get_builddir() / "html" / "nrf" / "doxygen"

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_projects = {
    "nrf": {
        "doxyfile": NRF_BASE / "doc" / "nrf" / "nrf.doxyfile.in",
        "outdir": _doxyrunner_outdir,
        "fmt": True,
        "fmt_vars": {
            "NRF_BASE": str(NRF_BASE),
            "DOCSET_SOURCE_BASE": str(NRF_BASE),
            "DOCSET_BUILD_DIR": str(_doxyrunner_outdir),
            "DOCSET_VERSION": version,
        }
    }
}

# create mbedtls config header (needed for Doxygen)
_doxyrunner_outdir.mkdir(exist_ok=True, parents=True)

fin_path = NRF_BASE / "subsys" / "nrf_security" / "configs" / "legacy_crypto_config.h.template"
fout_path = _doxyrunner_outdir / "mbedtls_doxygen_config.h"

with open(fin_path) as fin, open(fout_path, "w") as fout:
    fout.write(
        re.sub(
            r"#cmakedefine ([A-Z0-9_-]+)",
            r"#define \1",
            fin.read()
        )
    )

# -- Options for doxybridge plugin ---------------------------------------------

doxybridge_projects = {
    "nrf": _doxyrunner_outdir,
    "wifi": utils.get_builddir() / "html" / "wifi",
    "zephyr": utils.get_builddir() / "html" / "zephyr" / "doxygen",
}

# Options for ncs_include ------------------------------------------------------

ncs_include_mapping = {
    "nrf": utils.get_srcdir("nrf"),
    "nrfxlib": utils.get_srcdir("nrfxlib"),
    "zephyr": utils.get_srcdir("zephyr"),
}

# Options for html_redirect ----------------------------------------------------

html_redirect_pages = redirects.NRF

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

# -- Options for zephyr.gh_utils -----------------------------------------------

gh_link_version = "main" if version.endswith("99") else f"v{version}"
gh_link_base_url = f"https://github.com/nrfconnect/sdk-nrf"
gh_link_prefixes = {
    "applications/.*": "",
    "samples/.*": "",
    "scripts/.*": "",
    "tests/.*": "",
    ".*": "doc/nrf",
}


def setup(app):
    app.add_css_file("css/nrf.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

# Options for sphinx-llms-txt ---------------------------------------------------

llms_txt_summary = """
# nRF Connect SDK Documentation

Version: {version}

The nRF Connect SDK is Nordic Semiconductor's software development kit for
building products based on Nordic Semiconductor SoCs.

Quick start: gsg_guides.html
Installing the nRF Connect SDK: installation/install_ncs.html
"""

# DO NOT GENERATE llms-full.txt
llms_txt_full = False
llms_txt_exclude_patterns = [
    # EXCLUDE release notes - including latest tag
    "releases_and_maturity/releases/release-notes-{}".format(
    "changelog" if version.endswith("99") else version
    ),
]

# INCLUDE source files
llms_txt_source_files = [
]

# SET base URL - including latest tag
llms_txt_html_base_url = "https://docs.nordicsemi.com/bundle/ncs-{}/page/nrf/".format(
    "latest" if version.endswith("99") else version
)

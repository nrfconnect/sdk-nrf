#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from pathlib import Path
import sys


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

MATTER_BASE = utils.get_projdir("matter")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# General configuration --------------------------------------------------------

project = "Matter"
copyright = "2020-2023, Matter Contributors"
author = "Matter Contributors"
version = "1.0.0"

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "recommonmark",
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
html_title = "Matter documentation (nRF Connect SDK)"

html_theme_options = {
    "docset": "matter",
    "docsets": utils.ALL_DOCSETS,
    "subtitle": "nRF Connect SDK",
}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (NRF_BASE / "doc" / "matter", "*.rst"),
    (MATTER_BASE / "docs" / "guides", "images"),
    (MATTER_BASE / "docs" / "guides", "nrfconnect_platform_overview.md"),
    (MATTER_BASE / "docs" / "guides", "nrfconnect_examples_configuration.md"),
    (MATTER_BASE / "docs" / "guides", "nrfconnect_examples_cli.md"),
    (MATTER_BASE / "docs" / "guides", "nrfconnect_examples_software_update.md"),
    (MATTER_BASE / "docs" / "guides", "nrfconnect_factory_data_configuration.md"),
    (MATTER_BASE / "docs" / "guides", "BUILDING.md"),
    (MATTER_BASE / "docs" / "guides", "chip_tool_guide.md"),
    (MATTER_BASE / "docs" / "guides", "access-control-guide.md"),
]


def setup(app):
    app.add_css_file("css/matter.css")

    utils.add_google_analytics(app, html_theme_options)

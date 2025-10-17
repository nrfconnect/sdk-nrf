# Internal documentation build configuration file

import sys
from pathlib import Path

from sphinx.config import eval_config_file

# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

DOC_INTERNAL_BASE = utils.get_projdir("internal")
ZEPHYR_BASE = utils.get_projdir("zephyr")

# pylint: disable=undefined-variable

# General ----------------------------------------------------------------------

# Import internal configuration, override as needed later
conf = eval_config_file(str(DOC_INTERNAL_BASE / "conf.py"), tags)
locals().update(conf)

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))
extensions.extend(["zephyr.external_content"])

# Options for HTML output ------------------------------------------------------

html_theme_options = {"docset": "internal", "docsets": utils.ALL_DOCSETS}

# Options for external_content -------------------------------------------------

external_content_contents = [
    (DOC_INTERNAL_BASE, "**/*"),
]

# pylint: enable=undefined-variable


def setup(app):
    utils.add_google_analytics(app, html_theme_options)
    utils.add_announcement_banner(html_theme_options)

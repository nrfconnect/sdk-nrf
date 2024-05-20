#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from os import PathLike
from pathlib import Path
from typing import Tuple, Optional

from sphinx.application import Sphinx
from sphinx.cmd.build import get_parser
from west.manifest import Manifest


_NRF_BASE = Path(__file__).parents[2]
"""NCS Repository root"""

_MANIFEST = Manifest.from_file(_NRF_BASE / "west.yml")
"""Manifest instance"""

ALL_DOCSETS = {
    "nrf": ("nRF Connect SDK", "index", "manifest"),
    "nrfx": ("nrfx", "index", "hal_nordic"),
    "nrfxlib": ("nrfxlib", "README", "nrfxlib"),
    "zephyr": ("Zephyr Project", "index", "zephyr"),
    "mcuboot": ("MCUboot", "wrapper", "mcuboot"),
    "tfm": ("Trusted Firmware-M", "index", "trusted-firmware-m"),
    "matter": ("Matter", "index", "matter"),
    "kconfig": ("Kconfig Reference", "index", None),
}
"""All supported docsets (name: title, home page, manifest project name)."""


def get_projdir(docset: str) -> Path:
    """Obtain the project directory for the given docset.

    Args:
        docset: Target docset.

    Returns:
        Project path for the given docset.
    """

    name = ALL_DOCSETS[docset][2]
    if not name:
        raise ValueError("Given docset has no associated project")

    p = next((p for p in _MANIFEST.projects if p.name == name), None)
    assert p, f"Project {name} not in manifest"

    return Path(p.topdir) / Path(p.path)


def get_builddir() -> PathLike:
    """Obtain Sphinx base build directory for a given docset.

    Returns:
        Base build path.
    """
    parser = get_parser()
    args = parser.parse_args()
    return (Path(args.outputdir) / ".." / "..").resolve()


def get_outputdir(docset: str) -> PathLike:
    """Obtain Sphinx output directory for a given docset.

    Args:
        docset: Target docset.

    Returns:
        Build path of the given docset.
    """

    return get_builddir() / "html" / docset


def get_srcdir(docset: str) -> PathLike:
    """Obtain sources directory for a given docset.

    Args:
        docset: Target docset.

    Returns:
        Sources directory of the given docset.
    """

    return get_builddir() / docset / "src"


def get_intersphinx_mapping(docset: str) -> Optional[Tuple[str, str]]: # pylint: disable=unsubscriptable-object
    """Obtain intersphinx configuration for a given docset.

    Args:
        docset: Target docset.

    Notes:
        Relative links are used for URL prefix.

    Returns:
        Intersphinx configuration if available.
    """

    outputdir = get_outputdir(docset)
    inventory = outputdir / "objects.inv"
    if not inventory.exists():
        return

    return (str(Path("..") / docset), str(inventory))


def add_google_analytics(app: Sphinx, options: dict) -> None:
    """Add Google Analytics to a docset.

    Args:
        app: Sphinx instance.
        options: HTML theme options
    """

    app.add_js_file("js/gtm-insert.js")
    app.add_js_file(
        "https://policy.app.cookieinformation.com/uc.js",
        id="CookieConsent",
        type="text/javascript",
        **{"data-culture": "EN"},
    )

    options["add_gtm"] = True
    options["gtm_id"] = "GTM-WF4CVFX"

def add_announcement_banner(options: dict) -> None:
    """Add an announcement banner to the top of the page.
    Args:
        options: html theme options.
    """

    msg = "<b>Important: </b>This website will be taken offline by 2024-06-18 and replaced " \
           "with <a href=\"https://docs.nordicsemi.com/\">docs.nordicsemi.com</a>, " \
           "where all Nordic technical documentation is already accessible. " \

    options["set_default_announcement"] = True
    options["default_announcement_message"] = msg

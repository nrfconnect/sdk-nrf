#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from os import PathLike
from pathlib import Path

from sphinx.application import Sphinx
from sphinx.cmd.build import get_parser
from west.manifest import Manifest

_NRF_BASE = Path(__file__).parents[2]
"""NCS Repository root"""

_MANIFEST = Manifest.from_file(_NRF_BASE / "west.yml")
"""Manifest instance"""

ALL_DOCSETS = {
    "nrf": ("nRF Connect SDK", "index", "manifest"),
    "nrfxlib": ("nrfxlib", "README", "nrfxlib"),
    "zephyr": ("Zephyr Project", "index", "zephyr"),
    "mcuboot": ("MCUboot", "wrapper", "mcuboot"),
    "oberon": ("Oberon PSA Crypto", "wrapper", "oberon-psa-crypto"),
    "tfm": ("Trusted Firmware-M", "wrapper", "trusted-firmware-m"),
    "matter": ("Matter", "index", "matter"),
    "kconfig": ("Kconfig Reference", "index", None),
}
"""All supported docsets (name: title, home page, manifest project name)."""

OPTIONAL_DOCSETS = {
    "internal": ("Internal Documentation", "index", "doc-internal"),
}
"""Optional docsets (name: title, home page, manifest project name)."""


# append optional docsets if they exist
for docset, props in OPTIONAL_DOCSETS.items():
    for p in _MANIFEST.projects:
        if p.name != props[2] or not (Path(p.topdir) / Path(p.path)).exists():
            continue

        ALL_DOCSETS[docset] = props


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


def get_intersphinx_mapping(docset: str) -> tuple[str, str] | None:
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

    msg = ""

    options["set_default_announcement"] = False
    options["default_announcement_message"] = msg

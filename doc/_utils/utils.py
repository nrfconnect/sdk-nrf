from os import PathLike
from pathlib import Path
from sphinx.cmd.build import get_parser
from typing import Dict, Tuple, Optional


ALL_DOCSETS = {
    "nrf": ("nRF Connect SDK", "index"),
    "nrfx": ("nrfx", "index"),
    "nrfxlib": ("nrfxlib", "README"),
    "zephyr": ("Zephyr Project", "index"),
    "mcuboot": ("MCUboot", "wrapper"),
    "kconfig": ("Kconfig Reference", "index"),
}
"""All supported docsets (name: title, home page)."""


def get_docsets(docset: str) -> Dict[str, str]:
    """Obtain all docsets that should be displayed.

    Args:
        docset: Target docset.

    Returns:
        Dictionary of docsets.
    """
    docsets = ALL_DOCSETS.copy()
    del docsets[docset]
    return docsets


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


def get_intersphinx_mapping(docset: str) -> Optional[Tuple[str, str]]:
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

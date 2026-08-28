"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import sys
from datetime import datetime
from pathlib import Path

from bs4 import BeautifulSoup
from bs4.element import Tag

_BASE_URL = "https://nrfconnectdocs.nordicsemi.com/ncs/latest/{docset}/"


def create_urlset(soup: BeautifulSoup) -> Tag:
    return soup.new_tag(
        "sitemapindex", attrs={"xmlns": "http://www.sitemaps.org/schemas/sitemap/0.9"}
    )


def create_loc(tag: Tag, url: str) -> Tag:
    loc = tag.new_tag("loc")
    loc.string = url
    return loc


def create_lastmod(tag: Tag) -> Tag:
    lastmod = tag.new_tag("lastmod")

    lastmod.string = datetime.now().strftime("%Y-%m-%d")
    return lastmod


def create_sitemap(tag: Tag, url: str) -> Tag:
    sitemap = tag.new_tag("sitemap")
    tag.append(sitemap)

    sitemap.append(create_loc(sitemap, url))
    sitemap.append(create_lastmod(sitemap))

    return sitemap


def main() -> int:
    parser = argparse.ArgumentParser(
        prog=__file__,
        description="Create sitemap_index.xml from sitemaps generated for each docset during build",
        allow_abbrev=False,
    )

    parser.add_argument(
        "--html-dir",
        type=Path,
        default=Path("_build/html"),
        help="Path to the html output directory",
    )

    parser.add_argument("--base-url", type=str, default=_BASE_URL, help="Base url for sitemap")

    parser.add_argument(
        "sitemap",
        type=Path,
        nargs="?",
        default=Path("_build/html/sitemap_index.xml"),
        help="Output file path",
    )

    args = parser.parse_args()

    soup = BeautifulSoup()
    urlset = create_urlset(soup)
    soup.append(urlset)

    for path in args.html_dir.glob("**/sitemap.xml"):
        docset = path.parent.name
        urlset.append(create_sitemap(urlset, args.base_url.format(docset=docset)))

    with open(args.sitemap, "w") as f:
        f.write(soup.prettify())
    return 0


if __name__ == "__main__":
    sys.exit(main())

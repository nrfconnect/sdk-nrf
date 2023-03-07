"""
NCS Documentation Cache Upload Utility
======================================

This script can be used to upload documentation cache files created by the
cache_create.py script.

Usage
*****

python cache_upload.py --input $INPUT_DIR --key $AZURE_BLOB_KEY

Copyright (c) 2021 Nordic Semiconductor ASA
"""

import argparse
import logging
import os
from pathlib import Path
import re

from azure.storage.blob import ContainerClient


AZ_CONN_STR = ";".join(
    (
        "DefaultEndpointsProtocol=https",
        "EndpointSuffix=core.windows.net",
        "AccountName=ncsdocsa",
        "AccountKey={key}",
    )
)
"""Azure connection string."""

AZ_CONTAINER = "ncs-doc-container01"
"""Azure container."""


log = logging.getLogger(__name__)


def main(args) -> None:
    """Entry point.

    Args:
        args: CLI arguments.
    """

    cc = ContainerClient.from_connection_string(
        AZ_CONN_STR.format(key=args.key), AZ_CONTAINER
    )

    cache_files = [b.name for b in cc.list_blobs()]
    for cache_file in Path(args.input).iterdir():
        m = re.match(r"([a-z]+)-([a-f0-9]+)\.zip", cache_file.name)
        if not m:
            log.info(f"Skipping {cache_file} (not a cache file)")
            continue

        docset = m.group(1)
        if args.only and docset not in args.only:
            continue

        if not args.force and cache_file.name in cache_files:
            log.info(f"Skipping upload of {cache_file.name} (already exists)")
            continue

        with open(cache_file, "rb") as f:
            log.info(f"Uploading {cache_file.name}...")
            bc = cc.get_blob_client(cache_file.name)
            bc.upload_blob(f, overwrite=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "-i",
        "--input",
        type=Path,
        required=True,
        help="Path where cache files are found",
    )

    parser.add_argument(
        "--key",
        type=str,
        default=os.environ.get("NCS_CACHE_AZ_KEY"),
        help="Azure Storage Key",
    )

    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="Force upload operation (will overwrite existing files)",
    )

    parser.add_argument(
        "--only",
        nargs="+",
        help="Only upload cache for the given docsets",
    )

    args = parser.parse_args()

    sh = logging.StreamHandler()
    sh.setFormatter(logging.Formatter("%(message)s"))
    log.addHandler(sh)
    log.setLevel(logging.INFO)

    main(args)

"""
NCS Documentation Cache Creation Utility
======================================

This script can be used to create documentation cache files.
Note that upload should be restricted to automated systems, e.g. CI, in order
to guarantee it is always performed after a successful build.

Usage
*****

python cache_upload.py -o $OUTPUT_DIR -b $DOC_BUILD_DIR

Cache details
*************

nRF Connect SDK (NCS) documentation is composed by multiple docset. Each docset
is generated from source files gathered from multiple locations: NCS, a module
or a combination of both. Therefore, the documentation output created for a
particular docset depends directly on all the input sources. While the build
system knows this information, it is not exposed in a structured manner. This is
the reason we need a separate configuration file that specified which docsets we
have and on what they depend (see ``cache.yml`` for details).

In order to compute the cache information for a particular docset, the following
steps are performed:

1. All docset dependencies (i.e. module repositories) are gathered
2. Each docset is given a hash (SHA256) result of hashing all the dependencies
   ``HEAD`` revisions.

The following directory hierarchy is assumed::

    $(DOC_BUILD_DIR)
    |- $(docset)         # Docset build files (e.g. Doxygen, external content...)
    |- html/$(docset)    # HTML build output (also contains doctrees)

Copyright (c) 2021 Nordic Semiconductor ASA
"""

import argparse
import hashlib
from itertools import chain
import logging
from pathlib import Path
import pickle
import shutil
import zipfile
from typing import Optional, List, Dict

from pygit2 import Repository
from sphinx.application import ENV_PICKLE_FILENAME
from west.manifest import Manifest
import yaml

try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


REPO_ROOT = Path(__file__).absolute().parent / ".." / ".."
"""Repository root."""

ENV_CACHE_PICKLE_FILENAME = (
    Path(ENV_PICKLE_FILENAME).stem + "-cache" + Path(ENV_PICKLE_FILENAME).suffix
)
"""Cache environment file."""


log = logging.getLogger(__name__)


def get_docset_ids(
    config: Dict, manifest: Manifest, only: Optional[List[str]]
) -> Dict[str, str]:
    """Obtain the ids for all requested docsets.

    Args:
        config: Cache configuration.
        manifest: Repository manifest.
        only: Only consider this docset (null to consider all)

    Returns:
        State.
    """

    ids = dict()

    for entry in config:
        if only and entry["docset"] not in only:
            continue

        id = ""
        for name in entry["projects"]:
            p = next((p for p in manifest.projects if p.name == name), None)
            assert p, f"Project {name} not in manifest"

            repo = Repository(Path(p.topdir) / p.path)
            id += repo.revparse_single("HEAD").id.hex

        ids[entry["docset"]] = hashlib.sha256(id.encode("utf-8")).hexdigest()

    return ids


def make_relative_set(items: Dict, basedir: Path) -> None:
    """Make absolute paths relative to basedir and flag them with '*' prefix.

    Args:
        items: Dictionary with items -> set of entries
        basedir: Base directory.
    """

    for doc, set_entries in items.items():
        patched_set = set()
        for set_entry in set_entries:
            if set_entry is not None:
                p = Path(set_entry)
                if p.is_absolute():
                    try:
                        set_entry = f"*{p.relative_to(basedir).as_posix()}"
                    except ValueError:
                        # paths outside of basedir ignored, these are non-portable
                        continue
            patched_set.add(set_entry)
        items[doc] = patched_set


def patch_environment(env_file: Path, topdir: Path) -> None:
    """Patch Sphinx environment.

    Args:
        env_file: Environment file.
        topdir: Manifest top directory.
    """

    with open(env_file, "rb") as f:
        env = pickle.load(f)

    # make absolute paths relative:
    # we assume all paths will be relative to the manifest top dir, i.e.
    # contained within the workspace
    make_relative_set(env.included, topdir)
    make_relative_set(env.dependencies, topdir)

    env_cache = {
        "all_docs": env.all_docs,
        "dependencies": env.dependencies,
        "domaindata": env.domaindata,
        "files_to_rebuild": env.files_to_rebuild,
        "included": env.included,
        "longtitles": env.longtitles,
        "metadata": env.metadata,
        "titles": env.titles,
        "toc_num_entries": env.toc_num_entries,
        "tocs": env.tocs,
    }

    if hasattr(env, "doxyrunner_cache"):
        env_cache["doxyrunner_cache"] = env.doxyrunner_cache

    with open(env_file, "wb") as f:
        pickle.dump(env_cache, f)


def main(args) -> None:
    """Entry point.

    Args:
        args: CLI arguments.
    """

    config = yaml.load(open(args.config).read(), Loader=Loader)
    manifest = Manifest.from_file(args.manifest)
    ids = get_docset_ids(config, manifest, args.only)

    output = Path(args.output)
    output.mkdir(exist_ok=True)

    for docset, id in ids.items():
        log.info(f"Creating cache files for {docset}...")
        with open(output / f"{docset}-{id}.zip", "wb") as f:
            with zipfile.ZipFile(f, "w", zipfile.ZIP_DEFLATED) as zf:
                docset_files = chain(
                        args.build_dir.glob(f"{docset}/**/*"),
                        args.build_dir.glob(f"html/{docset}/**/*"),
                )
                for file in docset_files:
                    if file.name == ENV_CACHE_PICKLE_FILENAME:
                        continue

                    if file.name == ENV_PICKLE_FILENAME:
                        file_cache = file.parent / ENV_CACHE_PICKLE_FILENAME
                        shutil.copy(file, file_cache)
                        patch_environment(file_cache, Path(manifest.topdir))
                        file = file_cache

                    arcname = file.relative_to(args.build_dir)
                    zf.write(file, arcname)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="Path where cache files will be created.",
    )

    parser.add_argument(
        "-b",
        "--build-dir",
        type=Path,
        required=True,
        help="Documentation build directory",
    )

    parser.add_argument(
        "-c",
        "--config",
        type=Path,
        default=REPO_ROOT / "doc" / "cache.yml",
        help="Path to configuration file",
    )

    parser.add_argument(
        "-m",
        "--manifest",
        default=REPO_ROOT / "west.yml",
        type=Path,
        help="Manifest file",
    )

    parser.add_argument(
        "--only",
        nargs="+",
        help="Only create cache files for the given docsets",
    )

    args = parser.parse_args()

    sh = logging.StreamHandler()
    sh.setFormatter(logging.Formatter("%(message)s"))
    log.addHandler(sh)
    log.setLevel(logging.INFO)

    main(args)

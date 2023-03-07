"""
Sphinx search index merge utility
=================================

This script can be used to merge search indexes of multiple docsets, thus
allowing to obtain a cross-search experience.

Credits go to Dominik Kilian for the original idea and part of this code.

Usage
*****

python merge_search_indexes.py -b path/to/doc/build/dir

Copyright (c) 2021 Nordic Semiconductor ASA
"""

import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import sys
from typing import Dict


sys.path.insert(0, str(Path(__file__).absolute().parents[1] / "_utils"))
import utils


def load_search_index(file: Path, prefix: str) -> Dict:
    """Load search index from a file

    Args:
        file: Index file.
        prefix: Index prefix name.

    Returns:
        Search index.
    """

    with open(file) as f:
        m = re.match(r"^Search\.setIndex\((.*)\)$", f.read())
        if not m:
            raise ValueError(f"Unexpected search index content for {file}")

    escaped = re.sub(r"([{,])([A-Za-z0-9_]+):", r'\1"\2":', m.group(1))

    index = json.loads(escaped)
    index["titles"] = [f"{prefix} » {title}" for title in index["titles"]]

    return index


def dump_search_index(index: Dict, dst: Path) -> None:
    """Dump a search index to a file.

    Args:
        index: Search index.
        dst: Destination file.
    """

    with open(dst, "w") as f:
        f.write("Search.setIndex(")
        f.write(json.dumps(index))
        f.write(");")


def merge_doc_file_names(src: Dict, dst: Dict, src_docset: str) -> None:
    """Merge docnames and filenames entries.

    Args:
        src: Source index.
        dst: Destination index.
        src_docset: Source index docset name.
    """

    for docname in src["docnames"]:
        dst["docnames"].append(f"../{src_docset}/{docname}")
    for filename in src["filenames"]:
        dst["filenames"].append(f"../{src_docset}/{filename}")

    dst["titles"] += src["titles"]


def merge_terms(src: Dict, dst: Dict, offset: int) -> None:
    """Merge terms entries.

    This function merges the terms or titleterms fields of a source index into
    a destination index. Entries from source index are padded with the provided
    offset so that they point to the correct document index.

    Args:
        src: Source index.
        dst: Destination index.
        offset: Offset to be applied to the source index entries.
    """

    for key in ("terms", "titleterms"):
        src_entry = src[key]
        dst_entry = dst[key]

        for term, values in src_entry.items():
            try:
                existing = dst_entry[term]
            except KeyError:
                existing = list()

            if not isinstance(existing, list):
                existing = [existing]

            if not isinstance(values, list):
                values = [values]

            dst_entry[term] = existing + [value + offset for value in values]


def merge_objects(src: Dict, dst: Dict, offset: int) -> None:
    """Merge objects entries

    Args:
        src: Source index.
        dst: Destination index.
        offset: Offset to be applied to the source index entries.
    """

    # merge objnames and objtypes entries
    index_cnt = len(dst["objnames"]) - 1
    obj_map = dict()
    for src_index, src_value in src["objnames"].items():
        found = False
        for dst_index, dst_value in dst["objnames"].items():
            if (
                src_value == dst_value
                and src["objtypes"][src_index] == dst["objtypes"][dst_index]
            ):
                obj_map[src_index] = dst_index
                found = True
                break

        if not found:
            index_cnt += 1
            obj_map[src_index] = index_cnt
            dst["objnames"][str(index_cnt)] = src_value
            dst["objtypes"][str(index_cnt)] = src["objtypes"][src_index]

    # merge objects
    for src_prefix, src_objects in src["objects"].items():
        if src_prefix not in dst["objects"]:
            dst["objects"][src_prefix] = list()

        dst_objects = dst["objects"][src_prefix]
        for src_object in src_objects:
            dst_objects.append([
                src_object[0] + offset,
                obj_map[str(src_object[1])],
                src_object[2],
                src_object[3],
                src_object[4],
            ])


def main(build_dir: Path) -> None:
    """Entry point

    Args:
        build_dir: Documentation build directory.
    """

    # discover built docsets
    docsets = list()
    for entry in (build_dir / "html").iterdir():
        if entry.is_dir() and entry.name in utils.ALL_DOCSETS:
            docsets.append(entry.name)

    # load indexes
    indexes = dict()
    for docset in docsets:
        index_file = build_dir / "html" / docset / "searchindex.orig.js"
        if not index_file.exists():
            shutil.copy(build_dir / "html" / docset / "searchindex.js", index_file)

        indexes[docset] = load_search_index(index_file, utils.ALL_DOCSETS[docset][0])

    # merge indexes
    for docset, index in indexes.items():
        final_index = copy.deepcopy(index)
        for docset_to_merge, index_to_merge in indexes.items():
            if docset_to_merge == docset:
                continue

            offset = len(final_index["docnames"])

            merge_doc_file_names(index_to_merge, final_index, docset_to_merge)
            merge_terms(index_to_merge, final_index, offset)
            merge_objects(index_to_merge, final_index, offset)

        dump_search_index(final_index, build_dir / "html" / docset / "searchindex.js")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "-b",
        "--build-dir",
        type=Path,
        required=True,
        help="Documentation build directory",
    )

    args = parser.parse_args()

    main(args.build_dir)

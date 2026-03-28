#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Mirror 'a' support to 'b' in sdk-nrf:
- TARGET_A: nrf54lm20dk/nrf54lm20a/cpuapp
- TARGET_B: nrf54lm20dk/nrf54lm20b/cpuapp
- TOKEN_A:  nrf54lm20dk_nrf54lm20a_cpuapp
- TOKEN_B:  nrf54lm20dk_nrf54lm20b_cpuapp

Fix: Preserve YAML indentation by performing TEXTUAL insertion for list items.
ruamel.yaml is only used to analyze where insertions are required and to ensure idempotency.

Behavior:
1) Starting at --root (or current directory), recursively scan for 'sample.yaml' and 'testcase.yaml'.
2) For any YAML list that contains TARGET_A and does not contain TARGET_B:
   - Insert TARGET_B immediately after the first occurrence of TARGET_A by directly
     inserting a new line in the original file text with the SAME leading whitespace
     as the TARGET_A '- ' line. This preserves formatting/comments/indentation.
3) If a YAML was modified:
   - Recursively scan the YAML's directory and all subdirectories for files whose
     basename contains TOKEN_A.
   - Copy to same directory with TOKEN_A -> TOKEN_B in the basename. Never overwrite (warn+skip).
   - After copying:
       * For '.overlay' files, replace all occurrences of TOKEN_A -> TOKEN_B in content.
       * For all newly created files, if TOKEN_A appears in a non-'.overlay' file, warn (no changes).

Idempotent: re-running causes no further changes.

CLI:
  python mirror_lm20a_to_lm20b.py [--root PATH] [--dry-run] [--verbose | --quiet]
"""

from __future__ import annotations

import argparse
import sys
import os
import re
from pathlib import Path
import shutil
from typing import Any, Generator, Iterable, List, Tuple, Union, Optional

# ---- Constants ----
TARGET_A = "nrf54lm20dk/nrf54lm20a/cpuapp"
TARGET_B = "nrf54lm20dk/nrf54lm20b/cpuapp"
TOKEN_A = "nrf54lm20dk_nrf54lm20a_cpuapp"
TOKEN_B = "nrf54lm20dk_nrf54lm20b_cpuapp"

YAML_FILENAMES = {"sample.yaml", "testcase.yaml"}

# ---- ruamel.yaml (analysis only) ----
try:
    from ruamel.yaml import YAML
    from ruamel.yaml.comments import CommentedSeq, CommentedMap
except Exception:
    print(
        "ERROR: 'ruamel.yaml' is required. Install it with: pip install ruamel.yaml",
        file=sys.stderr,
    )
    sys.exit(1)

# ---- Simple logger ----
class Logger:
    def __init__(self, verbose: bool = False, quiet: bool = False) -> None:
        self.verbose = verbose
        self.quiet = quiet

    def info(self, msg: str) -> None:
        if not self.quiet:
            print(msg)

    def debug(self, msg: str) -> None:
        if self.verbose and not self.quiet:
            print(msg)

    def warn(self, msg: str) -> None:
        print(f"WARNING: {msg}", file=sys.stderr)

    def error(self, msg: str) -> None:
        print(f"ERROR: {msg}", file=sys.stderr)


# ---- YAML helpers (analysis only) ----
def yaml_load_round_trip(path: Path) -> Any:
    yaml = YAML(typ="rt")  # round-trip to get line/column info
    yaml.preserve_quotes = True
    with path.open("r", encoding="utf-8") as f:
        return yaml.load(f)


def is_sequence(node: Any) -> bool:
    return isinstance(node, (list, tuple, CommentedSeq))


def is_mapping(node: Any) -> bool:
    return isinstance(node, (dict, CommentedMap))


def walk_yaml_sequences(node: Any, path: List[Union[str, int]] | None = None) -> Generator[Tuple[List[Union[str, int]], CommentedSeq], None, None]:
    """
    Recursively traverse a YAML structure and yield (path, seq) for each sequence.
    """
    if path is None:
        path = []

    if is_sequence(node):
        yield (path, node)  # type: ignore
        for idx, val in enumerate(node):
            child_path = path + [idx]
            if is_sequence(val) or is_mapping(val):
                yield from walk_yaml_sequences(val, child_path)

    elif is_mapping(node):
        for k, v in node.items():
            child_path = path + [k]
            if is_sequence(v) or is_mapping(v):
                yield from walk_yaml_sequences(v, child_path)


def path_to_str(path: List[Union[str, int]]) -> str:
    parts: List[str] = []
    for p in path:
        if isinstance(p, int):
            parts[-1] = f"{parts[-1]}[{p}]"
        else:
            parts.append(str(p))
    return ".".join(parts) if parts else "(root)"


def plan_insertions(doc: Any) -> List[Tuple[List[Union[str, int]], int]]:
    """
    Analyze the YAML document and plan insertions.

    Returns a list of (seq_path, a_index) for sequences that:
      - contain TARGET_A,
      - do NOT contain TARGET_B,
      - and for which we will insert TARGET_B immediately after the first TARGET_A.
    """
    plans: List[Tuple[List[Union[str, int]], int]] = []
    for p, seq in walk_yaml_sequences(doc):
        if not isinstance(seq, CommentedSeq):
            # Support plain lists too
            pass
        as_strs = [str(x) for x in seq]
        if TARGET_A in as_strs and TARGET_B not in as_strs:
            a_index = as_strs.index(TARGET_A)
            plans.append((p, a_index))
    return plans


# ---- Filesystem helpers ----
def iter_yaml_files(root: Path) -> Iterable[Path]:
    for dirpath, dirnames, filenames in os.walk(root):
        for name in filenames:
            if name in YAML_FILENAMES:
                yield Path(dirpath) / name


def is_text_file(path: Path) -> bool:
    try:
        with path.open("r", encoding="utf-8") as f:
            f.read(2048)
        return True
    except Exception:
        return False


def read_text(path: Path) -> str:
    with path.open("r", encoding="utf-8", errors="replace") as f:
        return f.read()


def write_text(path: Path, content: str) -> None:
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write(content)


# ---- Textual insertion to preserve indentation ----
_A_ITEM_RE = re.compile(rf'^(\s*)-\s*{re.escape(TARGET_A)}\s*(#.*)?$')

def find_a_item_lines(lines: List[str]) -> List[int]:
    """Return all line indices where a YAML list item exactly equals TARGET_A."""
    matches: List[int] = []
    for i, line in enumerate(lines):
        if _A_ITEM_RE.match(line.rstrip("\n")):
            matches.append(i)
    return matches


def insert_b_after_a_lines(yaml_path: Path, a_line_numbers: List[int], dry_run: bool, logger: Logger) -> int:
    """
    Insert '- TARGET_B' immediately after the lines at the given indices.
    Preserve indentation by copying the captured leading whitespace from the 'A' line.
    Avoid duplicate insertion if the next line is already the exact '- TARGET_B' line.

    Returns the number of insertions performed.
    """
    if not a_line_numbers:
        return 0

    content = read_text(yaml_path)
    lines = content.splitlines(keepends=True)

    # Sort line numbers descending so indexes remain valid after insertions.
    a_line_numbers_sorted = sorted(set(a_line_numbers), reverse=True)
    insert_count = 0

    for idx in a_line_numbers_sorted:
        if idx < 0 or idx >= len(lines):
            continue
        m = _A_ITEM_RE.match(lines[idx].rstrip("\n"))
        if not m:
            # Fallback: if the exact match is not found (shouldn't happen), skip
            continue
        indent = m.group(1) or ""
        new_line = f"{indent}- {TARGET_B}\n"

        # If the next non-EOF line already equals our target, skip (idempotent guard).
        if idx + 1 < len(lines) and lines[idx + 1] == new_line:
            logger.debug(f"    Skipping duplicate insertion after line {idx+1} in {yaml_path}")
            continue

        logger.info(f"  YAML textual insert: line {idx+2} -> adding '{new_line.rstrip()}'")
        if not dry_run:
            lines.insert(idx + 1, new_line)
        insert_count += 1

    if insert_count and not dry_run:
        write_text(yaml_path, "".join(lines))
    return insert_count


def plan_and_apply_yaml_insertions(yaml_path: Path, dry_run: bool, logger: Logger) -> Tuple[bool, int]:
    """
    Analyze YAML (idempotently) and apply textual insertions preserving indentation.
    Returns (modified, num_lists_modified)
    """
    try:
        doc = yaml_load_round_trip(yaml_path)
    except Exception as e:
        logger.error(f"Failed to parse YAML: {yaml_path} ({e})")
        raise

    plans = plan_insertions(doc)
    if not plans:
        logger.debug("  No lists require insertion.")
        return False, 0

    # Build a map of line numbers for 'A' items that need B inserted after them.
    # Prefer precise line numbers derived from the sequence location data.
    a_line_numbers: List[int] = []
    # Attempt to use ruamel's line/col info from the sequences
    for seq_path, a_index in plans:
        # Navigate to the sequence
        node = doc
        for part in seq_path:
            node = node[part]  # type: ignore[index]
        seq = node  # this should be a list/CommentedSeq
        line_for_item: Optional[int] = None
        try:
            # ruamel keeps per-item line/col info on the sequence's .lc.data
            # Typically it's a list-like structure with tuples (line, col)
            lc = getattr(seq, "lc", None)
            data = getattr(lc, "data", None)
            if data is not None and a_index < len(data):
                item_lc = data[a_index]
                if isinstance(item_lc, (list, tuple)) and len(item_lc) >= 1:
                    line_for_item = int(item_lc[0])  # 0-based
        except Exception:
            line_for_item = None

        if line_for_item is not None:
            a_line_numbers.append(line_for_item)
        else:
            # Fallback: search textually for a matching '- TARGET_A' line
            file_lines = read_text(yaml_path).splitlines(keepends=False)
            for i, line in enumerate(file_lines):
                if _A_ITEM_RE.match(line):
                    a_line_numbers.append(i)
                    break
            else:
                logger.warn(f"  Could not locate line for TARGET_A in {yaml_path}; skipping this insertion.")

    # Apply textual insertions preserving indentation
    inserted = insert_b_after_a_lines(yaml_path, a_line_numbers, dry_run, logger)

    # Report per-list modifications (number of lists that needed insertion)
    num_lists = len(plans) if inserted else 0
    return inserted > 0, num_lists


# ---- Neighbourhood copy & post-processing ----
def perform_neighbourhood_copy(yaml_path: Path, dry_run: bool, logger: Logger) -> Tuple[int, int, int]:
    """
    From the YAML's directory, recursively copy all files whose basename contains TOKEN_A
    to a sibling with TOKEN_B in the basename. Never overwrite; warn and skip.
    After copying:
      - If new file ends with .overlay, replace all occurrences of TOKEN_A -> TOKEN_B in content.
      - For all new files, scan content; if TOKEN_A appears in a non-.overlay file, warn (no changes).

    Returns: (num_copies, num_overlay_replacements, num_warnings)
    """
    base_dir = yaml_path.parent
    num_copies = 0
    num_overlay_replacements = 0
    num_warnings = 0

    for dirpath, dirnames, filenames in os.walk(base_dir):
        for name in filenames:
            if TOKEN_A in name:
                src = Path(dirpath) / name
                dst_name = name.replace(TOKEN_A, TOKEN_B)
                dst = Path(dirpath) / dst_name

                if dst.exists():
                    num_warnings += 1
                    logger.warn(f"Destination exists, skipping copy: {dst}")
                    continue

                logger.info(f"  COPY: {src} -> {dst}")
                if not dry_run:
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(src, dst)
                num_copies += 1

                ext = dst.suffix.lower()
                if ext == ".overlay":
                    if not dry_run:
                        content = read_text(dst)
                        count_before = content.count(TOKEN_A)
                        if count_before > 0:
                            write_text(dst, content.replace(TOKEN_A, TOKEN_B))
                            num_overlay_replacements += count_before
                            logger.info(f"    Replaced {count_before} occurrence(s) in overlay: {dst}")
                else:
                    if not dry_run and is_text_file(dst):
                        content = read_text(dst)
                        if TOKEN_A in content:
                            num_warnings += 1
                            logger.warn(f"TOKEN_A found in non-overlay file: {dst} (no changes made)")

    return num_copies, num_overlay_replacements, num_warnings


# ---- Main ----
def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Mirror 'a' support to 'b' in sdk-nrf YAMLs and related files (indentation-safe).")
    parser.add_argument(
        "--root",
        default=str(Path.cwd()),
        help="Starting directory to scan recursively (default: current working directory)."
    )
    parser.add_argument("--dry-run", action="store_true", help="Print planned changes but do not write files.")
    verbosity = parser.add_mutually_exclusive_group()
    verbosity.add_argument("--verbose", action="store_true", help="Verbose logging.")
    verbosity.add_argument("--quiet", action="store_true", help="Quiet mode (warnings/errors only).")

    args = parser.parse_args(argv)

    root: Path = Path(args.root).resolve()
    logger = Logger(verbose=args.verbose, quiet=args.quiet)

    logger.debug(f"Start root: {root}")
    logger.debug(f"Dry-run: {args.dry_run}")

    if not root.exists() or not root.is_dir():
        logger.error(f"--root is not a directory: {root}")
        return 1

    yaml_files = list(iter_yaml_files(root))
    if not yaml_files:
        logger.info("No 'sample.yaml' or 'testcase.yaml' files found under the specified root.")
        return 0

    had_errors = False
    total_yaml_processed = 0
    total_yaml_modified = 0
    total_lists_modified = 0

    total_files_copied = 0
    total_overlay_replacements = 0
    total_warnings = 0

    for yml in yaml_files:
        total_yaml_processed += 1
        logger.info(f"\n=== YAML: {yml} ===")

        try:
            modified, lists_modified = plan_and_apply_yaml_insertions(yml, args.dry_run, logger)
        except Exception:
            had_errors = True
            continue

        if modified:
            total_yaml_modified += 1
            total_lists_modified += lists_modified
            copies, overlay_repl, warns = perform_neighbourhood_copy(yml, args.dry_run, logger)
            total_files_copied += copies
            total_overlay_replacements += overlay_repl
            total_warnings += warns
        else:
            logger.info("  No YAML changes required; skipping neighbourhood copy.")

    # Summary
    print("\n===== SUMMARY =====")
    print(f"Root:                    {root}")
    print(f"YAML files processed:     {total_yaml_processed}")
    print(f"YAML files modified:      {total_yaml_modified}")
    print(f"Lists modified (insert):  {total_lists_modified}")
    print(f"Files copied:             {total_files_copied}")
    print(f"Overlay replacements:     {total_overlay_replacements}")
    print(f"Warnings:                 {total_warnings}")
    print(f"Dry-run:                  {args.dry_run}")

    return 1 if had_errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

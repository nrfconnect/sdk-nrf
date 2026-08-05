#!/usr/bin/env python3
"""Inject varied Doxygen content-checker issues into header files for PR testing.

Usage:
    ./scripts/inject_doxygen_test_errors.py [--force]

Creates 50 files with distinct Doxygen mistake patterns (one per file).
Idempotent: skips files that already contain DOXYGEN_TEST_INJECT unless --force.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MARKER = "DOXYGEN_TEST_INJECT"
FILE_COUNT = 50
RANDOM_SEED = 0xD0C70001

SEARCH_DIRS = ("include", "subsys", "drivers", "applications", "lib")
SKIP_PARTS = ("/tests/", "/mock", "/babblesim/")

INJECT_BLOCK_RE = re.compile(
    r"\n/\* DOXYGEN_TEST_INJECT.*?(?=\n#endif|\Z)",
    re.DOTALL,
)


def collect_headers() -> list[Path]:
    headers: list[Path] = []
    for dirname in SEARCH_DIRS:
        base = ROOT / dirname
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*.h")):
            rel = path.as_posix()
            if any(part in rel for part in SKIP_PARTS):
                continue
            rel_path = path.relative_to(ROOT)
            text = path.read_text(encoding="utf-8", errors="replace")
            if "#endif" not in text and not text.rstrip().endswith("#pragma once"):
                continue
            headers.append(rel_path)
    return headers


def stable_rank(path: Path) -> int:
    digest = hashlib.sha256(f"{RANDOM_SEED}:{path}".encode()).digest()
    return int.from_bytes(digest[:8], "big")


def symbol_suffix(path: Path) -> str:
    digest = hashlib.sha256(path.as_posix().encode()).hexdigest()
    stem = re.sub(r"[^A-Za-z0-9_]", "_", path.stem.upper())[:16]
    return f"{stem}_{digest[:6].upper()}"


def build_injectors(suffix: str, lower: str) -> list[str]:
    """Fifty distinct snippets targeting different checker outcomes."""
    s, l = suffix, lower
    return [
        # --- missing documentation: macros ---
        f"#define DOXY_PR_TEST_{s} 1",
        f"#define DOXY_PR_TEST_{s}_MASK 0xFFU",
        f"#define DOXY_PR_TEST_{s}_VERSION 42",
        f"#define DOXY_PR_TEST_{s}_ENABLE 1",
        f"#define DOXY_PR_TEST_{s}_TIMEOUT_MS 500",
        f"#define DOXY_PR_TEST_{s}_FLAG (1U << 3)",
        f"#define DOXY_PR_TEST_{s}_MAX_LEN 128",
        f"#define DOXY_PR_TEST_{s}_DEFAULT 0",
        # --- missing documentation: functions ---
        f"void doxy_pr_test_{l}(void);",
        f"int doxy_pr_test_{l}_init(void);",
        f"uint32_t doxy_pr_test_{l}_read(void);",
        f"bool doxy_pr_test_{l}_ready(void);",
        f"int doxy_pr_test_{l}_write(const void *buf, size_t len);",
        f"void doxy_pr_test_{l}_reset(void);",
        f"ssize_t doxy_pr_test_{l}_poll(int timeout_ms);",
        f"const char *doxy_pr_test_{l}_name_get(void);",
        # --- missing documentation: struct / enum ---
        f"struct doxy_pr_test_{l} {{\n\tint value;\n}};",
        f"struct doxy_pr_test_{l}_ctx {{\n\tuint8_t *data;\n\tsize_t len;\n}};",
        f"enum doxy_pr_test_{l} {{\n\tDOXY_PR_TEST_{s}_IDLE = 0,\n\tDOXY_PR_TEST_{s}_BUSY,\n}};",
        f"enum doxy_pr_test_{l}_mode {{\n\tDOXY_PR_TEST_{s}_MODE_A = 1,\n\tDOXY_PR_TEST_{s}_MODE_B = 2,\n}};",
        # --- incomplete function documentation ---
        "/**\n * @file\n */\n" f"void doxy_pr_test_{l}_file_tag(void);",
        "/**\n *\n */\n" f"int doxy_pr_test_{l}_empty_brief(void);",
        "/** */\n" f"void doxy_pr_test_{l}_one_line(void);",
        "/**\n * @param[in] value Input only mentioned, no brief.\n */\n"
        f"int doxy_pr_test_{l}_param_only(int value);",
        "/**\n * @return Always zero without describing the API.\n */\n"
        f"int doxy_pr_test_{l}_return_only(void);",
        "/**\n * @retval 0\n */\n" f"int doxy_pr_test_{l}_retval_only(void);",
        "/**\n * @note Orphan note without brief.\n */\n" f"void doxy_pr_test_{l}_note_only(void);",
        "/**\n * @warning\n */\n" f"void doxy_pr_test_{l}_warning_only(void);",
        # --- regular C comment instead of Doxygen ---
        f"/* Plain C comment instead of Doxygen */\nvoid doxy_pr_test_{l}_c_comment(void);",
        f"/* TODO: document later */\nint doxy_pr_test_{l}_todo_comment(void);",
        f"/* FIXME: replace with proper docs */\nbool doxy_pr_test_{l}_fixme_comment(void);",
        # --- documented macro but wrong / odd (still missing for macro rules) ---
        f"/* non-doxygen prelude */\n#define DOXY_PR_TEST_{s}_PLAIN 7",
        # --- combo: two missing symbols in one file ---
        f"#define DOXY_PR_TEST_{s}_PAIR_A 1\n#define DOXY_PR_TEST_{s}_PAIR_B 2",
        f"void doxy_pr_test_{l}_pair_fn(void);\nint doxy_pr_test_{l}_pair_fn2(int x);",
        f"struct doxy_pr_test_{l}_pair {{\n\tint a;\n}};\n"
        f"enum doxy_pr_test_{l}_pair_e {{\n\tDOXY_PR_TEST_{s}_PAIR_E = 0,\n}};",
        # --- more function shapes ---
        f"void doxy_pr_test_{l}_cb(void (*handler)(int));",
        f"struct doxy_pr_test_{l}_opaque *doxy_pr_test_{l}_alloc(void);",
        f"int doxy_pr_test_{l}_multi(int a, int b, int c);",
        f"unsigned int doxy_pr_test_{l}_flags_get(void);",
        f"void doxy_pr_test_{l}_vla(size_t n, int values[n]);",
        # --- enum / struct variants ---
        f"enum doxy_pr_test_{l}_state {{\n\tDOXY_PR_TEST_{s}_OFF = 0,\n\tDOXY_PR_TEST_{s}_ON = 1,\n"
        f"\tDOXY_PR_TEST_{s}_ERR = -1,\n}};",
        f"struct doxy_pr_test_{l}_stats {{\n\tuint32_t packets;\n\tuint32_t errors;\n}};",
        f"struct doxy_pr_test_{l}_cfg {{\n\tbool enabled;\n\tuint8_t priority;\n}};",
        # --- incomplete blocks with different shapes ---
        "/**\n * @file\n * @defgroup doxy_pr_test_" + l + " Internal test group\n * @{\n */\n"
        f"void doxy_pr_test_{l}_open_group(void);",
        "/**\n * @internal\n */\n" f"void doxy_pr_test_{l}_internal_tag(void);",
        "/**\n * @deprecated\n */\n" f"void doxy_pr_test_{l}_deprecated_no_desc(void);",
        "/**\n * @see some_random_symbol\n */\n" f"void doxy_pr_test_{l}_see_only(void);",
        "/**\n * @todo Add docs\n */\n" f"int doxy_pr_test_{l}_todo_tag(void);",
        "/**\n * @param[in] a First\n * @param[out] b Second\n */\n"
        f"int doxy_pr_test_{l}_params_no_brief(int a, int b);",
        f"/* Leading block comment */\n/* Second line */\nvoid doxy_pr_test_{l}_multi_c_comment(void);",
        f"#define DOXY_PR_TEST_{s}_EXPR (DOXY_PR_TEST_{s}_A + DOXY_PR_TEST_{s}_B)",
        f"#define DOXY_PR_TEST_{s}_STRING \"injected\"",
    ]


def content_issue_snippet(path: Path, file_index: int) -> str:
    suffix = symbol_suffix(path)
    lower = suffix.lower()
    injectors = build_injectors(suffix, lower)
    return injectors[file_index % len(injectors)]


def build_block(path: Path, file_index: int) -> str:
    snippet = content_issue_snippet(path, file_index)
    return "\n".join(
        [
            "",
            "/* DOXYGEN_TEST_INJECT - fake content issues for reviewer testing, do not merge */",
            snippet,
            "",
        ]
    )


def find_insert_pos(lines: list[str]) -> int:
    for i in range(len(lines) - 1, -1, -1):
        if lines[i].strip().startswith("#endif"):
            return i
    return len(lines)


def strip_existing_inject(text: str) -> str:
    return INJECT_BLOCK_RE.sub("", text)


def inject_file(path: Path, file_index: int, force: bool) -> bool:
    full = ROOT / path
    try:
        text = full.read_text(encoding="utf-8")
    except OSError:
        return False

    if MARKER in text:
        if not force:
            return False
        text = strip_existing_inject(text)

    block = build_block(path, file_index)
    lines = text.splitlines(keepends=True)
    insert_at = find_insert_pos(lines)
    lines.insert(insert_at, block if block.endswith("\n") else block + "\n")
    try:
        full.write_text("".join(lines), encoding="utf-8")
    except OSError:
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--force",
        action="store_true",
        help="Replace an existing DOXYGEN_TEST_INJECT block in a file",
    )
    args = parser.parse_args()

    headers = collect_headers()
    ranked = sorted(headers, key=stable_rank)
    if len(ranked) < FILE_COUNT:
        raise SystemExit(f"Need at least {FILE_COUNT} injectable headers, found {len(ranked)}")

    changed = 0
    for index, path in enumerate(ranked[:FILE_COUNT]):
        if inject_file(path, index, args.force):
            changed += 1
            print(f"{index:02d}: {path}")

    print(f"\nModified {changed} files ({FILE_COUNT} distinct injectors)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3

# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""
Token-free quarantine notifier (diff-driven) with strict mode + audit JSON.

INPUTS:
  --diff-file diff_quarantine.txt        # file with diff (with context, e.g., -U100)
  --repo-root .                          # repo root for scanning YAML tests and CODEOWNERS
  --ref <sha>                            # head sha for blob URLs in the comment
  --strict-missing-codeowners            # mark violation when any affected scenario lacks owners
  --strict-flag-file strict_missing_codeowners.flag
  --audit-json quarantine_audit.json     # JSON summary output (owners, scenarios, etc.)
  --inventory-json scenario_inventory.json  # JSON map: scenario -> [yaml_paths]

OUTPUTS:
  * quarantine_comment.md  # Markdown body to post
  * quarantine_audit.json  # Full machine-readable summary for auditing
  * scenario_inventory.json
  * strict_missing_codeowners.flag  # (only when strict mode enabled AND violations found)

No GitHub API calls here; the workflow will post the comment and upload artifacts.
"""

import argparse
import json
import os
import re
import sys
from collections.abc import Iterable
from pathlib import Path

try:
    import yaml  # PyYAML
except Exception:
    print("ERROR: PyYAML is required (pip install pyyaml).", file=sys.stderr)
    raise

# ---------- CLI ----------


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=("Prepare quarantine owners notification comment from a diff."),
        allow_abbrev=False,
    )
    p.add_argument("--repo-root", default=".", help="Repository root (default: .)")
    p.add_argument(
        "--diff-file",
        required=True,
        help="Unified diff file of scripts/quarantine.yaml",
    )
    p.add_argument(
        "--output", default="quarantine_comment.md", help="Output Markdown file with comment body."
    )
    p.add_argument(
        "--audit-json", default="quarantine_audit.json", help="Audit summary JSON output path."
    )
    p.add_argument(
        "--inventory-json",
        default="scenario_inventory.json",
        help="Scenario inventory JSON output path.",
    )
    p.add_argument(
        "--ref",
        default=os.environ.get("GITHUB_SHA", "main"),
        help="Git ref/sha used for blob links in comment (default: env GITHUB_SHA or 'main').",
    )
    p.add_argument(
        "--strict-missing-codeowners",
        action="store_true",
        help="If any affected scenario has no CODEOWNERS, mark strict violation.",
    )
    p.add_argument(
        "--strict-flag-file",
        default="strict_missing_codeowners.flag",
        help="Path to flag file created when strict violation occurs.",
    )
    return p.parse_args()


# ---------- Diff parsing (find scenario patterns added/removed) ----------

DIFF_SKIP_PREFIXES = ("diff --git ", "index ", "--- ", "+++ ", "@@")


def parse_diff_for_scenarios(diff_text: str) -> tuple[set[str], set[str]]:
    """
    Parse unified diff (with context, e.g. -U100) and extract values
    added/removed under 'scenarios:' YAML keys.
    Returns: (added_patterns, removed_patterns)
    """
    added: set[str] = set()
    removed: set[str] = set()

    in_scenarios = False
    in_platforms = False
    scenarios_for_platforms = set()  # scenarios for the current platform

    DIFF_SKIP_PREFIXES = ("diff --git ", "index ", "--- ", "+++ ", "@@")

    def get_prefix_and_body(line: str) -> tuple[str, str]:
        if not line:
            return "", ""
        ch = line[0]
        if ch in "+- ":
            return ch, line[1:]
        return "", line  # shouldn't happen in a standard unified diff

    def strip_inline_comment(val: str) -> str:
        # remove trailing " # ..." comment fragments (unquoted)
        i = val.find(" #")
        return val[:i] if i != -1 else val

    def clean_value(val: str) -> str:
        v = strip_inline_comment(val.strip())
        if (v.startswith("'") and v.endswith("'")) or (v.startswith('"') and v.endswith('"')):
            v = v[1:-1]
        return v.strip()

    for raw in diff_text.splitlines():
        if any(raw.startswith(pfx) for pfx in DIFF_SKIP_PREFIXES):
            continue

        prefix, body = get_prefix_and_body(raw)
        # DO NOT pre-trim a leading space from body; measure indent as-is
        trimmed = body.lstrip()

        if not trimmed or trimmed.startswith("#"):
            continue
        if trimmed.startswith("comment:"):
            # reset all flags and go to next dictionary item
            if prefix == "+":
                added.update(scenarios_for_platforms)
            elif prefix == "-":
                removed.update(scenarios_for_platforms)

            in_scenarios = False
            in_platforms = False
            scenarios_for_platforms = set()
            continue

        # Enter scenarios block whenever we see a 'scenarios:' key (context, +, or -)
        if re.fullmatch(r"- scenarios:\s*", trimmed):
            in_scenarios = True
            in_platforms = False
            scenarios_for_platforms = set()
            continue
        elif re.fullmatch(r"- platforms:\s*", trimmed) or re.fullmatch(r"^platforms:\s*", trimmed):
            in_scenarios = False
            in_platforms = True
            continue

        # Collect list items within scenarios
        if in_scenarios or in_platforms:
            m = re.match(r"^-\s+(.*)$", trimmed)
            if m:
                val = clean_value(m.group(1))
                if not val:
                    continue
                if in_scenarios:
                    if prefix == "+":
                        added.add(val)
                    elif prefix == "-":
                        removed.add(val)
                    scenarios_for_platforms.add(val)
                elif in_platforms:
                    if prefix == "+":
                        added.update(scenarios_for_platforms)
                    elif prefix == "-":
                        removed.update(scenarios_for_platforms)
                    else:
                        continue

    return added, removed


# ---------- Scenario discovery ----------

SCENARIO_YAML_GLOBS = [
    "samples/**/sample.yaml",
    "samples/**/testcase.yaml",
    "applications/**/sample.yaml",
    "applications/**/testcase.yaml",
    "tests/**/testcase.yaml",
    "tests/**/sample.yaml",
]


def discover_scenarios(repo_root: Path) -> dict[str, set[str]]:
    """
    Map: scenario_name -> set(yaml_paths_defining_it)
    Keys in top-level 'tests:' mapping of each YAML are Twister scenario names.
    """
    mapping: dict[str, set[str]] = {}
    for pattern in SCENARIO_YAML_GLOBS:
        for p in repo_root.glob(pattern):
            if not p.is_file():
                continue
            try:
                data = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
                tests = data.get("tests", {})
                if isinstance(tests, dict):
                    for scenario in tests:
                        s = str(scenario).strip()
                        if s:
                            rel = p.resolve().relative_to(repo_root.resolve()).as_posix()
                            mapping.setdefault(s, set()).add(rel)

            except Exception:
                continue
    return mapping


# ---------- CODEOWNERS parsing & matching ----------

CODEOWNERS_PATH = "CODEOWNERS"
CODEOWNER_LINE_RE = re.compile(r"^\s*([^\s#][^\s]*)\s+(.+?)\s*$")


def load_codeowners(repo_root: Path) -> list[tuple[str, list[str]]]:
    path = repo_root / CODEOWNERS_PATH
    if not path.exists():
        return []
    rules: list[tuple[str, list[str]]] = []
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        m = CODEOWNER_LINE_RE.match(s)
        if not m:
            continue
        pattern, owners_str = m.groups()
        owners = [tok for tok in owners_str.split() if tok.startswith("@")]
        if owners:
            rules.append((pattern, owners))
    return rules


def codeowners_pattern_to_regex(pattern: str) -> re.Pattern:
    anchored = pattern.startswith("/")
    pat = pattern[1:] if anchored else pattern

    pat = pat.replace("**/", "___GLOBSTAR_DIR___/")
    pat = pat.replace("/**", "/___GLOBSTAR_TAIL___")
    pat = pat.replace("**", "___GLOBSTAR___")

    pat = re.escape(pat)
    pat = pat.replace("___GLOBSTAR_DIR___", ".*")
    pat = pat.replace("___GLOBSTAR_TAIL___", ".*")
    pat = pat.replace("___GLOBSTAR___", ".*")
    pat = pat.replace(r"\*", "[^/]*").replace(r"\?", "[^/]")

    if pattern.endswith("/"):
        pat += ".*"

    if anchored:
        return re.compile("^" + pat + "$")
    else:
        return re.compile(r"(^|.*/)" + pat + r"($|/.*)")


def owners_for_path(path: str, rules: list[tuple[str, list[str]]]) -> list[str]:
    matched: list[str] | None = None
    for pat, owners in rules:
        if codeowners_pattern_to_regex(pat).search(path):
            matched = owners  # LAST match wins
    return matched or []


# ---------- Pattern expansion ----------


def compile_rx(pattern: str) -> re.Pattern:
    return re.compile(pattern + r"\Z")  # full-match semantics


def expand_patterns(patterns: Iterable[str], known_scenarios: Iterable[str]) -> set[str]:
    out: set[str] = set()
    compiled: list[tuple[str, re.Pattern]] = []
    for p in patterns:
        try:
            compiled.append((p, compile_rx(p)))
        except re.error:
            compiled.append((p, re.compile(re.escape(p) + r"\Z")))
    for name in known_scenarios:
        for _, rx in compiled:
            if rx.fullmatch(name):
                out.add(name)
                break
    return out


# ---------- Comment formatting ----------

COMMENT_MARKER = "<!-- quarantine-notifier -->"


def make_comment(
    owner_to_added: dict[str, list[tuple[str, str]]],
    owner_to_removed: dict[str, list[tuple[str, str]]],
    unowned_added: list[tuple[str, str]],
    unowned_removed: list[tuple[str, str]],
    repo_full: None | str,
    strict: bool,
) -> str:
    all_owner_keys = sorted(
        set(owner_to_added.keys()) | set(owner_to_removed.keys()), key=str.lower
    )
    any_owned = bool(all_owner_keys)
    any_unowned = bool(unowned_added or unowned_removed)
    if not any_owned and not any_unowned:
        return ""  # nothing to notify

    def link(path: str) -> str:
        return f"{path}" if repo_full else path

    def section(title: str, items: list[str], lines: list[str]):
        if items:
            lines.append(f"### {title}")
            lines.extend(items)
            lines.append("")

    lines: list[str] = []
    lines.append(COMMENT_MARKER)
    lines.append("**Quarantine update – notifying maintainers**\n")

    for key in all_owner_keys:
        owners = [o.strip() for o in key.split(",") if o.strip()]
        mention = ", ".join(owners) if owners else "_(no owners found)_"
        mention = mention if mention else "_(no owners found)_"
        lines.append(
            f"{mention}: Please take a note of quarantine changes for scenarios "
            f"under your maintainership."
        )
        add_lines, del_lines = [], []
        for scen, path in sorted(owner_to_added.get(key, [])):
            add_lines.append(f"- **Added to quarantine**: `{scen}` (defined in {link(path)})")
        for scen, path in sorted(owner_to_removed.get(key, [])):
            del_lines.append(f"- **Removed from quarantine**: `{scen}` (defined in {link(path)})")
        section("Added", add_lines, lines)
        section("Removed", del_lines, lines)
        lines.append("---")

    if any_unowned:
        header = "### ⚠️ Missing CODEOWNERS"
        if strict:
            header += " (strict mode enabled)"
        lines.append(header)
        if unowned_added:
            lines.append("**Added to quarantine – no owners resolved:**")
            for scen, path in sorted(unowned_added):
                lines.append(f"- `{scen}` (defined in {link(path)})")
        if unowned_removed:
            if unowned_added:
                lines.append("")
            lines.append("**Removed from quarantine – no owners resolved:**")
            for scen, path in sorted(unowned_removed):
                lines.append(f"- `{scen}` (defined in {link(path)})")
        if strict:
            lines.append("")
            lines.append(
                "> **Action required:** Please add appropriate CODEOWNERS entries "
                "for the paths above. This PR check will fail due to strict mode."
            )
        lines.append("---")

    return "\n".join(lines).strip() + "\n"


# ---------- Grouping & audit ----------


def resolve_codeowners_for_scenarios(
    scenario_to_paths: dict[str, set[str]],
    scenarios: Iterable[str],
    codeowners_rules: list[tuple[str, list[str]]],
) -> tuple[dict[str, list[tuple[str, str]]], list[tuple[str, str]]]:
    owners_map: dict[str, list[tuple[str, str]]] = {}
    unowned: list[tuple[str, str]] = []
    for scen in scenarios:
        for p in scenario_to_paths.get(scen, set()):
            owners = owners_for_path(p, codeowners_rules)
            if not owners:
                unowned.append((scen, p))
                continue
            key = ",".join(sorted(set(owners), key=str.lower))
            owners_map.setdefault(key, []).append((scen, p))
    return owners_map, unowned


def filter_neutral_scenarios(added: set[str], removed: set[str]) -> tuple[set[str], set[str]]:
    """Remove scenarios that appear in both added and removed sets."""
    neutral = added & removed
    return added - neutral, removed - neutral


def write_json(path: Path, obj: dict) -> None:
    path.write_text(json.dumps(obj, indent=2, sort_keys=False) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    root = Path(args.repo_root).resolve()
    repo_full = os.environ.get("GITHUB_REPOSITORY")

    diff_path = Path(args.diff_file)
    if not diff_path.exists():
        print(f"Diff file not found: {diff_path}", file=sys.stderr)
        Path(args.output).write_text("", encoding="utf-8")
        write_json(Path(args.audit_json), {"error": "diff_not_found"})
        write_json(Path(args.inventory_json), {})
        return 1

    diff_text = diff_path.read_text(encoding="utf-8", errors="ignore")
    added_patterns, removed_patterns = parse_diff_for_scenarios(diff_text)

    scenario_map = discover_scenarios(root)
    code_rules = load_codeowners(root)

    expanded_add = expand_patterns(sorted(added_patterns), scenario_map.keys())
    expanded_del = expand_patterns(sorted(removed_patterns), scenario_map.keys())
    expanded_add, expanded_del = filter_neutral_scenarios(expanded_add, expanded_del)

    # Resolve CODEOWNERS only for non-neutral scenarios
    owned_add, unowned_add = resolve_codeowners_for_scenarios(
        scenario_map, expanded_add, code_rules
    )
    owned_del, unowned_del = resolve_codeowners_for_scenarios(
        scenario_map, expanded_del, code_rules
    )

    body = make_comment(
        owner_to_added=owned_add,
        owner_to_removed=owned_del,
        unowned_added=unowned_add,
        unowned_removed=unowned_del,
        repo_full=repo_full,
        strict=args.strict_missing_codeowners,
    )
    Path(args.output).write_text(body, encoding="utf-8")

    # Inventory JSON (scenario -> [paths])
    write_json(Path(args.inventory_json), {k: sorted(v) for k, v in sorted(scenario_map.items())})

    # Audit JSON summary
    audit = {
        "repo": repo_full,
        "ref": args.ref,
        "quarantine_diff_file": str(diff_path),
        "added_patterns": sorted(added_patterns),
        "removed_patterns": sorted(removed_patterns),
        "expanded": {
            "added": sorted(list(expanded_add)),
            "removed": sorted(list(expanded_del)),
        },
        "owners": {
            key: {
                "added": [{"scenario": s, "path": p} for (s, p) in sorted(owned_add.get(key, []))],
                "removed": [
                    {"scenario": s, "path": p} for (s, p) in sorted(owned_del.get(key, []))
                ],
            }
            for key in sorted(set(owned_add.keys()) | set(owned_del.keys()), key=str.lower)
        },
        "unowned": {
            "added": [{"scenario": s, "path": p} for (s, p) in sorted(unowned_add)],
            "removed": [{"scenario": s, "path": p} for (s, p) in sorted(unowned_del)],
        },
        "stats": {
            "patterns_added": len(added_patterns),
            "patterns_removed": len(removed_patterns),
            "scenarios_added": len(expanded_add),
            "scenarios_removed": len(expanded_del),
            "owners_groups": len(set(owned_add.keys()) | set(owned_del.keys())),
            "unowned_items": len(unowned_add) + len(unowned_del),
        },
        "strict_mode": bool(args.strict_missing_codeowners),
        "strict_violation": False,
    }
    strict_violation = args.strict_missing_codeowners and (
        len(unowned_add) > 0 or len(unowned_del) > 0
    )
    if strict_violation:
        Path(args.strict_flag_file).write_text(
            "Missing CODEOWNERS detected for affected scenarios.\n", encoding="utf-8"
        )
        audit["strict_violation"] = True
        print(
            "Strict mode violation: missing CODEOWNERS found. Flag file created.", file=sys.stderr
        )

    write_json(Path(args.audit_json), audit)

    if body.strip():
        print("Prepared quarantine comment with maintainer mentions.")
    else:
        print("No content to post (no owners matched and no unowned items).")
    return 0


if __name__ == "__main__":
    sys.exit(main())


#!/usr/bin/env python3
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""
Token-free quarantine notifier (configuration-driven) with strict mode + audit JSON.

INPUTS:
  --added-file  configurations_added.txt   # lines: ("scenario","platform")
  --removed-file configurations_removed.txt # lines: ("scenario","platform")
  --repo-root .                            # repo root for scanning YAML tests and CODEOWNERS
  --ref <sha>                              # head sha for blob URLs in the comment
  --strict-missing-codeowners              # mark violation when any affected scenario lacks owners
  --strict-flag-file strict_missing_codeowners.flag
  --audit-json quarantine_audit.json       # JSON summary output (owners, scenarios, platforms, etc.)
  --inventory-json scenario_inventory.json # JSON map: scenario -> [yaml_paths]

OUTPUTS:
  * quarantine_comment.md                  # Markdown body to post
  * quarantine_audit.json                  # Full machine-readable summary for auditing
  * scenario_inventory.json
  * strict_missing_codeowners.flag         # (only when strict mode enabled AND violations found)

No GitHub API calls here; the workflow will post the comment and upload artifacts.
"""

import argparse
import ast
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


# ---------------- CLI ----------------
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=("Prepare quarantine owners notification comment from configuration files."),
        allow_abbrev=False,
    )
    p.add_argument("--repo-root", default=".", help="Repository root (default: .)")
    p.add_argument(
        "--added-file",
        required=True,
        help="Path to configurations_added.txt (lines: ('scenario','platform'))",
    )
    p.add_argument(
        "--removed-file",
        required=True,
        help="Path to configurations_removed.txt (lines: ('scenario','platform'))",
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


# ---------------- Diff parsing (legacy - unused now, kept untouched) ----------------
DIFF_SKIP_PREFIXES = ("diff --git ", "index ", "--- ", "+++ ", "@@")


# ---------------- Scenario discovery ----------------
SCENARIO_YAML_GLOBS = [
    "samples/**/*/sample.yaml",
    "samples/**/*/testcase.yaml",
    "applications/**/*/sample.yaml",
    "applications/**/*/testcase.yaml",
    "tests/**/*/testcase.yaml",
    "tests/**/*/sample.yaml",
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


# ---------------- CODEOWNERS parsing & matching ----------------
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

    # Globstar support and wildcard normalization
    pat = pat.replace("**/", "__GLOBSTAR_DIR__/")
    pat = pat.replace("/**", "/__GLOBSTAR_TAIL__")
    pat = pat.replace("**", "__GLOBSTAR__")

    pat = re.escape(pat)
    pat = pat.replace("__GLOBSTAR_DIR__", ".*")
    pat = pat.replace("__GLOBSTAR_TAIL__", ".*")
    pat = pat.replace("__GLOBSTAR__", ".*")
    pat = pat.replace(r"\*", r"[^/]*").replace(r"\?", r"[^/]")

    if pattern.endswith("/"):
        pat += ".*"

    if anchored:
        return re.compile("^" + pat + "$")
    else:
        # Match anywhere within a path segment boundary
        return re.compile(r"(^|.*/)" + pat + r"($|/.*)")


def owners_for_path(path: str, rules: list[tuple[str, list[str]]]) -> list[str]:
    matched: list[str] | None = None
    for pat, owners in rules:
        if codeowners_pattern_to_regex(pat).search(path):
            matched = owners  # LAST match wins
    return matched or []


# ---------------- Pattern expansion (legacy, kept untouched) ----------------
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


# ---------------- Comment formatting ----------------
COMMENT_MARKER = "<!-- quarantine-notifier -->"
ALL_PLATFORMS_TOKEN = "__ALL__"


def make_comment(
    owner_to_added: dict[str, list[tuple[str, str]]],
    owner_to_removed: dict[str, list[tuple[str, str]]],
    unowned_added: list[tuple[str, str]],
    unowned_removed: list[tuple[str, str]],
    repo_full: None | str,
    strict: bool,
    scenario_to_added_platforms: dict[str, set[str]],
    scenario_to_removed_platforms: dict[str, set[str]],
    platform_only_added: set[str],
    platform_only_removed: set[str],
) -> str:
    all_owner_keys = sorted(
        set(owner_to_added.keys()) | set(owner_to_removed.keys()), key=str.lower
    )
    any_owned = bool(all_owner_keys)
    any_unowned = bool(unowned_added or unowned_removed)

    if not any_owned and not any_unowned and not platform_only_added and not platform_only_removed:
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

        add_lines: list[str] = []
        del_lines: list[str] = []

        for scen, path in sorted(owner_to_added.get(key, [])):
            plats = scenario_to_added_platforms.get(scen, set())
            plat_str = (
                "all platforms"
                if ALL_PLATFORMS_TOKEN in plats
                else ", ".join(sorted(plats)) if plats else "-"
            )
            add_lines.append(
                f"- **Added to quarantine**: `{scen}` (platforms: {plat_str}) (defined in {link(path)})"
            )

        for scen, path in sorted(owner_to_removed.get(key, [])):
            plats = scenario_to_removed_platforms.get(scen, set())
            plat_str = (
                "all platforms"
                if ALL_PLATFORMS_TOKEN in plats
                else ", ".join(sorted(plats)) if plats else "-"
            )
            del_lines.append(
                f"- **Removed from quarantine**: `{scen}` (platforms: {plat_str}) (defined in {link(path)})"
            )

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
                plats = scenario_to_added_platforms.get(scen, set())
                plat_str = (
                    "all platforms"
                    if ALL_PLATFORMS_TOKEN in plats
                    else ", ".join(sorted(plats)) if plats else "-"
                )
                lines.append(f"- `{scen}` (platforms: {plat_str}) (defined in {link(path)})")

        if unowned_removed:
            if unowned_added:
                lines.append("")
            lines.append("**Removed from quarantine – no owners resolved:**")
            for scen, path in sorted(unowned_removed):
                plats = scenario_to_removed_platforms.get(scen, set())
                plat_str = (
                    "all platforms"
                    if ALL_PLATFORMS_TOKEN in plats
                    else ", ".join(sorted(plats)) if plats else "-"
                )
                lines.append(f"- `{scen}` (platforms: {plat_str}) (defined in {link(path)})")

        if strict:
            lines.append("")
            lines.append(
                "> **Action required:** Please add appropriate CODEOWNERS entries "
                "for the paths above. This PR check will fail due to strict mode."
            )
        lines.append("---")

    # Platform-only notices (scenario == None)
    platform_add_lines = [f"- Platform {p} is quarantined" for p in sorted(platform_only_added)]
    platform_del_lines = [f"- Platform {p} quarantine removed" for p in sorted(platform_only_removed)]
    section("Added (platform-only)", platform_add_lines, lines)
    section("Removed (platform-only)", platform_del_lines, lines)

    return "\n".join(lines).strip() + "\n"


# ---------------- Grouping & audit ----------------
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


def write_json(path: Path, obj: dict) -> None:
    path.write_text(json.dumps(obj, indent=2, sort_keys=False) + "\n", encoding="utf-8")


def load_configurations(path: Path) -> list[tuple[str | None, str | None]]:
    """
    Each non-empty line should look like: ("scenario","platform")
    Accepts 'None' (string) or actual None for either field.
    """
    if not path.exists():
        return []
    pairs: list[tuple[str | None, str | None]] = []
    for raw in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        try:
            t = ast.literal_eval(s)
        except Exception:
            m = re.match(r'^\(\s*"?(.*?)"?\s*,\s*"?(.*?)"?\s*\)\s*$', s)
            if not m:
                continue
            t = (m.group(1), m.group(2))
        scen_raw = t[0]
        plat_raw = t[1]
        scen = None if (scen_raw is None or str(scen_raw).strip() == "None") else str(scen_raw).strip()
        plat = None if (plat_raw is None or str(plat_raw).strip() == "None") else str(plat_raw).strip()
        pairs.append((scen, plat))
    return pairs


# ---------------- Main ----------------
def main() -> int:
    args = parse_args()
    root = Path(args.repo_root).resolve()
    repo_full = os.environ.get("GITHUB_REPOSITORY")

    added_path = Path(args.added_file)
    removed_path = Path(args.removed_file)
    if not added_path.exists() or not removed_path.exists():
        missing = []
        if not added_path.exists():
            missing.append(str(added_path))
        if not removed_path.exists():
            missing.append(str(removed_path))
        print(f"Configuration file(s) not found: {', '.join(missing)}", file=sys.stderr)
        Path(args.output).write_text("", encoding="utf-8")
        write_json(Path(args.audit_json), {"error": "config_files_not_found", "missing": missing})
        write_json(Path(args.inventory_json), {})
        return 1

    added_cfg = load_configurations(added_path)
    removed_cfg = load_configurations(removed_path)

    scenario_map = discover_scenarios(root)
    code_rules = load_codeowners(root)

    # Build scenario -> platforms maps and platform-only sets
    scenario_to_added_platforms: dict[str, set[str]] = {}
    scenario_to_removed_platforms: dict[str, set[str]] = {}
    platform_only_added: set[str] = set()
    platform_only_removed: set[str] = set()

    def add_pair(target_map: dict[str, set[str]], scen: str, plat: str | None):
        s = target_map.setdefault(scen, set())
        if plat is None:
            s.add(ALL_PLATFORMS_TOKEN)
        else:
            s.add(plat)

    for scen, plat in added_cfg:
        if scen is None and plat is not None:
            platform_only_added.add(plat)
        elif scen is None and plat is None:
            # ambiguous; ignore silently
            continue
        else:
            add_pair(scenario_to_added_platforms, scen, plat)

    for scen, plat in removed_cfg:
        if scen is None and plat is not None:
            platform_only_removed.add(plat)
        elif scen is None and plat is None:
            continue
        else:
            add_pair(scenario_to_removed_platforms, scen, plat)

    # Resolve CODEOWNERS for scenarios present in added/removed (ignore scenario == None)
    expanded_add = set(scenario_to_added_platforms.keys())
    expanded_del = set(scenario_to_removed_platforms.keys())

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
        scenario_to_added_platforms=scenario_to_added_platforms,
        scenario_to_removed_platforms=scenario_to_removed_platforms,
        platform_only_added=platform_only_added,
        platform_only_removed=platform_only_removed,
    )

    Path(args.output).write_text(body, encoding="utf-8")

    # Inventory JSON (scenario -> [paths])
    write_json(Path(args.inventory_json), {k: sorted(v) for k, v in sorted(scenario_map.items())})

    # Audit JSON summary
    audit = {
        "repo": repo_full,
        "ref": args.ref,
        "configurations_added_file": str(added_path),
        "configurations_removed_file": str(removed_path),
        "configurations": {
            "added": [
                {"scenario": s if s is not None else "None", "platform": p if p is not None else "None"}
                for (s, p) in added_cfg
            ],
            "removed": [
                {"scenario": s if s is not None else "None", "platform": p if p is not None else "None"}
                for (s, p) in removed_cfg
            ],
        },
        "expanded": {
            "added": sorted(list(expanded_add)),
            "removed": sorted(list(expanded_del)),
        },
        "owners": {
            key: {
                "added": [{"scenario": s, "path": p} for (s, p) in sorted(owned_add.get(key, []))],
                "removed": [{"scenario": s, "path": p} for (s, p) in sorted(owned_del.get(key, []))],
            }
            for key in sorted(set(owned_add.keys()) | set(owned_del.keys()), key=str.lower)
        },
        "unowned": {
            "added": [{"scenario": s, "path": p} for (s, p) in sorted(unowned_add)],
            "removed": [{"scenario": s, "path": p} for (s, p) in sorted(unowned_del)],
        },
        "stats": {
            "config_lines_added": len(added_cfg),
            "config_lines_removed": len(removed_cfg),
            "scenarios_added": len(expanded_add),
            "scenarios_removed": len(expanded_del),
            "platform_only_added": len(platform_only_added),
            "platform_only_removed": len(platform_only_removed),
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
        print("Prepared quarantine comment with maintainer mentions and platforms.")
    else:
        print("No content to post (no owners matched, no platform-only items, and no unowned items).")
    return 0


if __name__ == "__main__":
    sys.exit(main())

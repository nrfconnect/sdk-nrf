#!/usr/bin/env python3
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""
Quarantine notifier

INPUTS:
  --diff-dir <path>                        # compare_quarantine output dir
                                           # (configurations_added*.txt,
                                           # configurations_removed*.txt,
                                           # scenario_map.txt)
  --repo-root .                            # repo root for CODEOWNERS resolution
  --ref <sha>                              # head sha for blob URLs in the comment
  --output quarantine_comment.md           # Markdown file to write (default)

OUTPUTS:
  * quarantine_comment.md                  # Markdown body to post
  * <diff-dir>/configurations_added_combined.txt    # all added configurations (debug)
  * <diff-dir>/configurations_removed_combined.txt  # all removed configurations (debug)

No GitHub API calls here; the workflow will post the comment and upload artifacts.
"""

import argparse
import ast
import os
import sys
from collections.abc import Iterable
from pathlib import Path

import pathspec


# ---------------- CLI ----------------
def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=("Prepare quarantine owners notification comment from configuration files."),
        allow_abbrev=False,
    )
    p.add_argument("--repo-root", default=".", help="Repository root (default: .)")
    p.add_argument(
        "--diff-dir",
        required=True,
        help=(
            "Directory with compare_quarantine outputs: configurations_added*.txt, "
            "configurations_removed*.txt, and scenario_map.txt"
        ),
    )
    p.add_argument(
        "--output", default="quarantine_comment.md", help="Output Markdown file with comment body."
    )
    p.add_argument(
        "--ref",
        default=os.environ.get("GITHUB_SHA", "main"),
        help="Git ref/sha used for blob links in comment (default: env GITHUB_SHA or 'main').",
    )
    return p.parse_args()


# ---------------- CODEOWNERS parsing & matching ----------------
CODEOWNERS_PATH = "CODEOWNERS"
FIND_MY = "find_my"


def parse_codeowners_line(line: str) -> tuple[str, list[str]] | None:
    """Parse one CODEOWNERS line: '<pattern> @owner1 @owner2'."""
    s = line.strip()
    if not s or s.startswith("#"):
        return None
    tokens = s.split()
    if not tokens:
        return None
    pattern = tokens[0]
    owners = [token for token in tokens[1:] if token.startswith("@")]
    if not owners:
        return None
    return pattern, owners


def load_codeowners(repo_root: Path) -> list[tuple[str, list[str]]]:
    path = repo_root / CODEOWNERS_PATH
    if not path.exists():
        return []
    rules: list[tuple[str, list[str]]] = []
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        parsed = parse_codeowners_line(line)
        if parsed:
            rules.append(parsed)
    return rules


def compile_pathspecs(rules):
    compiled = []
    for pattern, owners in rules:
        spec = pathspec.PathSpec.from_lines("gitwildmatch", [pattern])
        compiled.append((spec, owners))
    return compiled


def find_owners(filepath: str, compiled_specs: list[tuple[str, list[str]]]) -> set[str]:
    matched_owners = set()
    for spec, owners in compiled_specs:
        if spec.match_file(filepath):
            matched_owners = owners
    return matched_owners


# ---------------- Comment formatting ----------------
COMMENT_MARKER = "<!-- quarantine-notifier -->"
ALL_PLATFORMS_TOKEN = "__ALL_PLATFORMS__"
ALL_SCENARIOS_TOKEN = "__ALL_SCENARIOS__"
ALL_PLATFORMS_LABEL = "all platforms"


def _format_plat_str(plats: set[str]) -> str:
    """Format a set of platform names for display."""
    if ALL_PLATFORMS_TOKEN in plats:
        return ALL_PLATFORMS_LABEL
    if plats:
        return ", ".join(sorted(plats))
    return "-"


def _format_scenario_entries(
    entries: list[tuple[str | None, str]],
    plat_map: dict[str | None, set[str]],
    link_fn,
) -> list[str]:
    """Format (scenario, path) pairs into Markdown list lines."""
    lines = []
    for scen, path in sorted(entries):
        plat_str = _format_plat_str(plat_map.get(scen, set()))
        label = "all scenarios" if scen == ALL_SCENARIOS_TOKEN else (scen or "")
        lines.append(f"- `{label}` (platforms: {plat_str}) defined in {link_fn(path)}")
    return lines


def _append_section(title: str, items: list[str], lines: list[str]) -> None:
    if items:
        lines.append(f"### {title}")
        lines.extend(items)
        lines.append("")


def make_comment(
    owner_to_added: dict[str, list[tuple[str | None, str]]],
    owner_to_removed: dict[str, list[tuple[str | None, str]]],
    unowned_added: list[tuple[str | None, str]],
    unowned_removed: list[tuple[str | None, str]],
    repo_full: None | str,
    scenario_to_added_platforms: dict[str | None, set[str]],
    scenario_to_removed_platforms: dict[str | None, set[str]],
    platform_only_added: set[str],
    platform_only_removed: set[str],
) -> str:
    all_owner_keys = sorted(
        set(owner_to_added.keys()) | set(owner_to_removed.keys()), key=str.lower
    )
    any_unowned = bool(unowned_added or unowned_removed)

    nothing = not all_owner_keys and not any_unowned
    if nothing and not platform_only_added and not platform_only_removed:
        return ""  # nothing to notify

    def link(path: str) -> str:
        return f"{path}" if repo_full else path

    lines: list[str] = []
    lines.append(COMMENT_MARKER)
    lines.append("**Quarantine update – notifying maintainers**\n")

    for key in all_owner_keys:
        owners = [o.strip() for o in key.split(",") if o.strip()]
        mention = ", ".join(owners) or "_(no owners found)_"
        lines.append(
            f"{mention}: Please take a note of quarantine changes for scenarios "
            f"under your maintainership."
        )
        add_lines = _format_scenario_entries(
            owner_to_added.get(key, []), scenario_to_added_platforms, link
        )
        del_lines = _format_scenario_entries(
            owner_to_removed.get(key, []), scenario_to_removed_platforms, link
        )
        _append_section("Added", add_lines, lines)
        _append_section("Removed", del_lines, lines)
        lines.append("---")

    if any_unowned:
        lines.append("### ⚠️ Missing CODEOWNERS")
        if unowned_added:
            lines.append("**Added to quarantine – no owners resolved:**")
            lines.extend(_format_scenario_entries(unowned_added, scenario_to_added_platforms, link))
        if unowned_removed:
            if unowned_added:
                lines.append("")
            lines.append("**Removed from quarantine – no owners resolved:**")
            lines.extend(
                _format_scenario_entries(unowned_removed, scenario_to_removed_platforms, link)
            )
        lines.append("---")

    platform_add_lines = [f"- Platform {p} is quarantined" for p in sorted(platform_only_added)]
    platform_del_lines = [
        f"- Platform {p} quarantine removed" for p in sorted(platform_only_removed)
    ]
    _append_section("Added (platform-only)", platform_add_lines, lines)
    _append_section("Removed (platform-only)", platform_del_lines, lines)

    return "\n".join(lines).strip() + "\n"


# ---------------- Grouping ----------------
def resolve_codeowners_for_scenarios(
    scenario_to_paths: dict[str, str],
    scenarios: Iterable[str | None],
    compiled_specs: list[tuple[pathspec.PathSpec, list[str]]],
) -> tuple[dict[str, list[tuple[str | None, str]]], list[tuple[str | None, str]]]:
    owners_map: dict[str, list[tuple[str | None, str]]] = {}
    unowned: list[tuple[str | None, str]] = []

    for scen in scenarios:
        if scen and FIND_MY in scen:
            owners = ["@nrfconnect/ncs-si-bluebagel"]
            path_full = "sdk-find-my repository (private)"
        elif scen == ALL_SCENARIOS_TOKEN:
            owners = ["@nrfconnect/ncs-test-leads"]
            path_full = "N/A (all scenarios)"
        else:
            path_full = scenario_to_paths.get(scen or "", "")
            if not path_full:
                unowned.append((scen, path_full))
                continue
            path_prefix, path = path_full.split("/", 1)
            if path_prefix != "nrf":
                owners = ["@nrfconnect/ncs-code-owners"]
            else:
                owners = find_owners(path, compiled_specs)
        if not owners:
            unowned.append((scen, path_full))
            continue
        key = ",".join(sorted(set(owners), key=str.lower))
        owners_map.setdefault(key, []).append((scen, path_full))

    return owners_map, unowned


ADDED_REPORT_PREFIX = "configurations_added"
REMOVED_REPORT_PREFIX = "configurations_removed"
COMBINED_ADDED_FILE = "configurations_added_combined.txt"
COMBINED_REMOVED_FILE = "configurations_removed_combined.txt"


def list_configuration_reports(diff_dir: Path, kind: str) -> list[Path]:
    """Return per-quarantine report files from compare_quarantine.py."""
    prefix = ADDED_REPORT_PREFIX if kind == "added" else REMOVED_REPORT_PREFIX
    skip = {
        diff_dir / COMBINED_ADDED_FILE,
        diff_dir / COMBINED_REMOVED_FILE,
    }
    return sorted(p for p in diff_dir.glob(f"{prefix}*.txt") if p not in skip)


def write_combined_configuration_reports(diff_dir: Path, kind: str, sources: list[Path]) -> Path:
    """Concatenate per-quarantine reports into one file for debugging."""
    name = COMBINED_ADDED_FILE if kind == "added" else COMBINED_REMOVED_FILE
    combined_path = diff_dir / name
    with combined_path.open("w", encoding="utf-8") as out:
        for src in sources:
            out.write(src.read_text(encoding="utf-8"))
    return combined_path


def combine_configuration_reports(paths: list[Path]) -> list[tuple[str | None, str | None]]:
    pairs: list[tuple[str | None, str | None]] = []
    for path in paths:
        pairs.extend(load_configurations(path))
    return pairs


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
        except (SyntaxError, ValueError):
            continue
        if not isinstance(t, tuple) or len(t) != 2:
            continue
        scen_raw = t[0]
        plat_raw = t[1]
        scen = (
            None if (scen_raw is None or str(scen_raw).strip() == "None") else str(scen_raw).strip()
        )
        plat = (
            None if (plat_raw is None or str(plat_raw).strip() == "None") else str(plat_raw).strip()
        )
        pairs.append((scen, plat))
    return pairs


def load_scenario_map(path: Path) -> dict[str, str]:
    """
    Load scenario map from compare_quarantine.py output.
    Each line: scenario: path1, path2
    """
    if not path.exists():
        return {}
    scenario_map: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = line.strip()
        if not s or ":" not in s:
            continue
        scen, loc = s.split(":", 1)
        scenario_map[scen.strip()] = loc.strip()
    return scenario_map


# ---------------- Main ----------------
def main() -> int:
    args = parse_args()
    root = Path(args.repo_root).resolve()
    repo_full = os.environ.get("GITHUB_REPOSITORY")
    diff_dir = Path(args.diff_dir).resolve()

    if not diff_dir.is_dir():
        print(f"Diff directory not found: {diff_dir}", file=sys.stderr)
        Path(args.output).write_text("", encoding="utf-8")
        return 1

    added_reports = list_configuration_reports(diff_dir, "added")
    removed_reports = list_configuration_reports(diff_dir, "removed")
    if not added_reports and not removed_reports:
        print(f"No configuration diff reports under {diff_dir}", file=sys.stderr)
        Path(args.output).write_text("", encoding="utf-8")
        return 0

    added_combined = write_combined_configuration_reports(diff_dir, "added", added_reports)
    removed_combined = write_combined_configuration_reports(diff_dir, "removed", removed_reports)
    added_cfg = combine_configuration_reports(added_reports)
    removed_cfg = combine_configuration_reports(removed_reports)
    print(
        f"Merged {len(added_reports)} added and {len(removed_reports)} removed report(s) "
        f"({len(added_cfg)} added, {len(removed_cfg)} removed configurations)."
    )
    print(f"Wrote: {added_combined}")
    print(f"Wrote: {removed_combined}")

    scenario_map_path = diff_dir / "scenario_map.txt"
    if not scenario_map_path.exists():
        print(f"Scenario map not found: {scenario_map_path}", file=sys.stderr)
        Path(args.output).write_text("", encoding="utf-8")
        return 1

    scenario_map = load_scenario_map(scenario_map_path)
    rules = load_codeowners(root)
    compiled_specs = compile_pathspecs(rules)

    scenario_to_added_platforms: dict[str | None, set[str]] = {}
    scenario_to_removed_platforms: dict[str | None, set[str]] = {}
    platform_only_added: set[str] = set()
    platform_only_removed: set[str] = set()

    def add_pair(target_map: dict[str | None, set[str]], scen: str | None, plat: str | None):
        s = target_map.setdefault(scen, set())
        if plat is None:
            s.add(ALL_PLATFORMS_TOKEN)
        else:
            s.add(plat)

    for scen, plat in added_cfg:
        if scen is None and plat is not ALL_PLATFORMS_TOKEN:
            platform_only_added.add(plat)
        else:
            add_pair(scenario_to_added_platforms, scen, plat)

    for scen, plat in removed_cfg:
        if scen is None and plat is not ALL_PLATFORMS_TOKEN:
            platform_only_removed.add(plat)
        else:
            add_pair(scenario_to_removed_platforms, scen, plat)

    owned_add, unowned_add = resolve_codeowners_for_scenarios(
        scenario_map, scenario_to_added_platforms, compiled_specs
    )
    owned_del, unowned_del = resolve_codeowners_for_scenarios(
        scenario_map, scenario_to_removed_platforms, compiled_specs
    )

    body = make_comment(
        owner_to_added=owned_add,
        owner_to_removed=owned_del,
        unowned_added=unowned_add,
        unowned_removed=unowned_del,
        repo_full=repo_full,
        scenario_to_added_platforms=scenario_to_added_platforms,
        scenario_to_removed_platforms=scenario_to_removed_platforms,
        platform_only_added=platform_only_added,
        platform_only_removed=platform_only_removed,
    )

    Path(args.output).write_text(body, encoding="utf-8")

    if body.strip():
        print("Prepared quarantine comment with maintainer mentions and platforms.")
    else:
        print(
            "No content to post (no owners matched, no platform-only items, and no unowned items)."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())

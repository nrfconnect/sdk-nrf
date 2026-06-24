#!/usr/bin/env python3

# Copyright (c) 2025 Zephyr Project
# SPDX-License-Identifier: Apache-2.0

"""Script to compare two quarantine.yaml files and report added/removed scenarios."""

import argparse
import os
import re
import sys
from collections.abc import Iterable
from itertools import product
from pathlib import Path

try:
    import yaml  # PyYAML
except Exception:
    print("ERROR: PyYAML is required (pip install pyyaml).", file=sys.stderr)
    raise

ZEPHYR_BASE = Path(os.getenv("ZEPHYR_BASE", ""))
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "pylib" / "twister" / "twisterlib"))
from quarantine import QuarantineData  # noqa: E402

ALL_PLATFORMS_TOKEN = "__ALL_PLATFORMS__"
ALL_SCENARIOS_TOKEN = "__ALL_SCENARIOS__"
FIND_MY = "find_my"

SCENARIO_YAML_GLOBS = [
    "**/samples/**/*/sample.yaml",
    "**/samples/**/*/testcase.yaml",
    "**/samples/**/*/tests.yaml",
    "**/applications/**/*/sample.yaml",
    "**/applications/**/*/testcase.yaml",
    "**/applications/**/*/tests.yaml",
    "**/tests/**/*/testcase.yaml",
    "**/tests/**/*/sample.yaml",
    "**/tests/**/*/tests.yaml",
]


def get_all_configurations(quarantine_file):
    """Extract all configurations from a quarantine file."""
    try:
        quarantine_data = QuarantineData.load_data_from_yaml(quarantine_file)
        configurations = set()

        for qelem in quarantine_data.qlist:
            scenarios = qelem.scenarios if qelem.scenarios else [ALL_SCENARIOS_TOKEN]
            platforms = qelem.platforms if qelem.platforms else [ALL_PLATFORMS_TOKEN]
            configurations.update(product(scenarios, platforms))
        return configurations
    except Exception as e:
        print(f"Error loading {quarantine_file}: {e}")
        sys.exit(1)


def expand_configurations(
    configurations: Iterable[tuple[str, str]], scenario_map: Iterable[str]
) -> set[tuple[str, str]]:
    """Expand configurations with scenario patterns to explicit scenario-platform pairs.

    Scenario entries are treated as regular expressions, matching Twister's
    quarantine semantics (re.compile + fullmatch).
    """
    expanded = set()
    scenarios = list(scenario_map)
    for scenario_pattern, platform in configurations:
        if scenario_pattern is ALL_SCENARIOS_TOKEN or FIND_MY in scenario_pattern:
            expanded.add((scenario_pattern, platform))
            continue
        try:
            regex = re.compile(scenario_pattern)
        except re.error as e:
            print(f"Warning: invalid scenario regex '{scenario_pattern}': {e}")
            continue
        matched_scenarios = {s for s in scenarios if regex.fullmatch(s)}
        if not matched_scenarios:
            print(f"Warning: pattern '{scenario_pattern}' did not match any scenarios.")
        for s in matched_scenarios:
            expanded.add((s, platform))
    return expanded


def _extract_scenarios_from_yaml(p: Path, repo_parent: Path) -> dict[str, str]:
    """Return {scenario_name: relative_path} for all scenarios defined in a YAML file."""
    try:
        data = yaml.safe_load(p.read_text(encoding="utf-8")) or {}
        tests = data.get("tests", {})
        if not isinstance(tests, dict):
            return {}
        rel = p.resolve().relative_to(repo_parent.resolve()).as_posix()
        return {str(s).strip(): rel for s in tests if str(s).strip()}
    except Exception as e:
        print(f"Error processing {p}: {e}")
        return {}


def discover_scenarios(repo_root: Path) -> dict[str, set[str]]:
    """
    Map: scenario_name -> set(yaml_paths_defining_it)
    Keys in top-level 'tests:' mapping of each YAML are Twister scenario names.
    """
    repo_parent = repo_root.parent
    mapping: dict[str, set[str]] = {}
    for pattern in SCENARIO_YAML_GLOBS:
        for p in repo_parent.glob(pattern):
            if not p.is_file():
                continue
            for scenario, rel_path in _extract_scenarios_from_yaml(p, repo_parent).items():
                mapping.setdefault(scenario, set()).add(rel_path)
    return mapping


def compare_quarantine_files(file1, file2, scenario_map):
    """Compare two quarantine files and return added/removed configurations."""
    print("Comparing quarantine files:")
    print(f"  File 1: {file1}")
    print(f"  File 2: {file2}")

    configurations1 = get_all_configurations(file1)
    configurations2 = get_all_configurations(file2)

    expanded_add = expand_configurations(sorted(set(configurations1)), scenario_map.keys())
    expanded_del = expand_configurations(sorted(set(configurations2)), scenario_map.keys())

    added_configurations = expanded_add - expanded_del
    removed_configurations = expanded_del - expanded_add
    return added_configurations, removed_configurations


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        allow_abbrev=False,
    )
    parser.add_argument("file1", type=Path, help="First quarantine file")
    parser.add_argument("file2", type=Path, help="Second quarantine file")
    parser.add_argument(
        "--outdir",
        type=Path,
        default=Path("."),
        help="Directory for output txt files",
    )
    parser.add_argument("--repo-root", default=".", help="Repository root (default: .)")
    return parser.parse_args(argv)


def _report_configurations(label: str, configs: set, symbol: str) -> None:
    """Print a summary of added or removed configurations."""
    if configs:
        print(f"Configurations {label} ({len(configs)}):")
        for config in sorted(configs):
            print(f"  {symbol} {config}")
    else:
        print(f"No configurations {label.lower()}.")
    print()


def main() -> int:
    args = parse_args()

    file1 = args.file1
    file2 = args.file2
    root = Path(args.repo_root).resolve()

    if file1.stem != file2.stem:
        print("Error: file1 and file2 must have the same stem.")
        return 1

    suffix = file1.stem.split("quarantine")[1]

    outdir = Path.cwd()
    if args.outdir:
        outdir = Path(args.outdir).resolve(strict=False)
        try:
            outdir.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            print(f"Error: unable to create output directory '{outdir}': {e}")
            return 1

    scenario_map = discover_scenarios(root)

    print(f"Writing reports to: {outdir}")
    added_configurations, removed_configurations = compare_quarantine_files(
        file1, file2, scenario_map
    )

    _report_configurations("REMOVED", removed_configurations, "-")
    _report_configurations("ADDED", added_configurations, "+")

    total_changes = len(added_configurations) + len(removed_configurations)
    if total_changes == 0:
        print("No changes detected between the files.")
    else:
        print(
            f"Total changes: {total_changes} "
            f"({len(added_configurations)} added, {len(removed_configurations)} removed)"
        )

    with open(outdir / f"configurations_added{suffix}.txt", "w", encoding="utf-8") as report_file:
        for config in sorted(added_configurations):
            report_file.write(f"{config}\n")
    with open(outdir / f"configurations_removed{suffix}.txt", "w", encoding="utf-8") as report_file:
        for config in sorted(removed_configurations):
            report_file.write(f"{config}\n")
    with open(outdir / "scenario_map.txt", "w", encoding="utf-8") as report_file:
        for scenario in sorted(scenario_map):
            report_file.write(f"{scenario}: {', '.join(scenario_map[scenario])}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())

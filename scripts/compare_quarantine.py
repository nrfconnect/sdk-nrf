#!/usr/bin/env python3

# Copyright (c) 2025 Zephyr Project
# SPDX-License-Identifier: Apache-2.0

"""
Script to compare two quarantine.yaml files and report added/removed scenarios.
"""

import argparse
import sys
from pathlib import Path
from itertools import product

# Add the twister library path to import Quarantine
import os
ZEPHYR_BASE = Path(os.getenv("ZEPHYR_BASE"))
sys.path.insert(0, str(ZEPHYR_BASE / 'scripts' / 'pylib' / 'twister' / 'twisterlib'))

from quarantine import QuarantineData


def get_all_configurations(quarantine_file):
    """Extract all configurations from a quarantine file."""
    try:
        quarantine_data = QuarantineData.load_data_from_yaml(quarantine_file)
        configurations = set()

        for qelem in quarantine_data.qlist:
            # Add all configurations from this quarantine element
            scenarios = qelem.scenarios if qelem.scenarios else [None]
            platforms = qelem.platforms if qelem.platforms else [None]
            # Generate all possible pairs
            configurations.update(product(scenarios, platforms))
        return configurations
    except Exception as e:
        print(f"Error loading {quarantine_file}: {e}")
        sys.exit(1)


def compare_quarantine_files(file1, file2):
    """Compare two quarantine files and return added/removed configurations."""
    print("Comparing quarantine files:")
    print(f"  File 1: {file1}")
    print(f"  File 2: {file2}")

    configurations1 = get_all_configurations(file1)
    configurations2 = get_all_configurations(file2)

    added_configurations = configurations2 - configurations1
    removed_configurations = configurations1 - configurations2
    return added_configurations, removed_configurations


if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(
        description="Compare two quarantine.yaml files and report added/removed configurations."
    )
    parser.add_argument("file1", type=Path, help="First quarantine file")
    parser.add_argument("file2", type=Path, help="Second quarantine file")
    parser.add_argument("--outdir", type=Path, default=Path("."), help="Directory for output txt files")
    
    args = parser.parse_args()
    
    file1 = args.file1
    file2 = args.file2
    outdir_arg = args.outdir

    if file1.stem != file2.stem:
        print("Error: file1 and file2 must have the same stem.")
        sys.exit(1)

    # Determine suffix for output files
    suffix = file1.stem.split("quarantine")[1]

    # Determine output directory: if provided, resolve relative paths against cwd
    # and create the directory (parents=True). If not provided, use current working dir.
    if outdir_arg:
        outdir = Path(outdir_arg).resolve(strict=False)
        try:
            outdir.mkdir(parents=True, exist_ok=True)
        except PermissionError:
            print(f"Error: insufficient permissions to create output directory '{outdir}'.")
            sys.exit(1)
        except Exception as e:
            print(f"Error: unable to create output directory '{outdir}': {e}")
            sys.exit(1)
    else:
        outdir = Path.cwd()

    print(f"Writing reports to: {outdir}")
    added_configurations, removed_configurations = compare_quarantine_files(file1, file2)

    # Report results
    if removed_configurations:
        print(f"Configurations REMOVED ({len(removed_configurations)}):")
        for config in sorted(removed_configurations):
            print(f"  - {config}")
        print()
    else:
        print("No configurations removed.")
        print()

    if added_configurations:
        print(f"Configurations ADDED ({len(added_configurations)}):")
        for config in sorted(added_configurations):
            print(f"  + {config}")
        print()
    else:
        print("No configurations added.")
        print()

    # Summary
    total_changes = len(added_configurations) + len(removed_configurations)
    if total_changes == 0:
        print("No changes detected between the files.")
    else:
        print(f"Total changes: {total_changes} ({len(added_configurations)} added, {len(removed_configurations)} removed)")

    with open(outdir / f"configurations_added{suffix}.txt", "w") as report_file:
        for config in sorted(added_configurations):
            report_file.write(f"{config}\n")    
    with open(outdir / f"configurations_removed{suffix}.txt", "w") as report_file:
        for config in sorted(removed_configurations):
            report_file.write(f"{config}\n")
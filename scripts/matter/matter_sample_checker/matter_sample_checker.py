#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
NCS Matter Sample Consistency Checker

This script checks the consistency of Matter samples in nRF Connect SDK.
It verifies file structure, configuration consistency, license years,
PM static files, ZAP files, and detects copy-paste mistakes.

Usage: python matter_sample_checker.py <sample_directory_path>
"""

import argparse
import os
import sys
from datetime import datetime
from pathlib import Path

from internal.check_discovery import CheckDiscovery
from internal.checker import MatterSampleChecker
from internal.utils.utils import load_config, parse_samples_zap_yaml


def main():
    parser = argparse.ArgumentParser(
        description='Check Matter sample consistency',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
        epilog="""
Examples:
  python matter_sample_checker.py /path/to/ncs/nrf/samples/matter/template
  python matter_sample_checker.py /path/to/ncs/nrf/samples/matter/light_bulb
  python matter_sample_checker.py  # Use current directory (year checking skipped by default)
  python matter_sample_checker.py --year  # Enable year checking for current year
  python matter_sample_checker.py --year 2024  # Check for 2024 copyright year
  python matter_sample_checker.py --year 2023 2024 2025  # Allow multiple years
  python matter_sample_checker.py -y  # Short form for current year
  python matter_sample_checker.py -y 2021 2022  # Short form with specific years
  python matter_sample_checker.py --config /path/to/custom_config.yaml  # Custom config
  python matter_sample_checker.py -c custom.yaml -y 2024  # Combined options
  python matter_sample_checker.py --allow-names "light bulb" template  # Allow specific names in \
copy-paste checking
  python matter_sample_checker.py -a "light bulb" "door lock" -v  # Allow multiple names with \
verbose output
  python matter_sample_checker.py --samples-zap-yaml zap_samples.yml --base /path/to/ncs/nrf
  python matter_sample_checker.py -s zap_samples.yml -b /path/to/ncs/nrf -y 2024
        """,
    )
    parser.add_argument(
        'sample_path',
        nargs='?',
        default='.',
        help='Path to the Matter sample directory to check \
                        (default: current directory). Ignored if --samples-zap-yaml is used.',
    )
    parser.add_argument(
        '--samples-zap-yaml',
        '-s',
        type=str,
        help='Path to YAML file containing list of samples to check \
                        (similar to zap_samples.yml format)',
    )
    parser.add_argument(
        '--base',
        '-n',
        type=str,
        help='Base directory for resolving sdk-nrf path. If not specified, \
                        paths are resolved using ZEPHYR_BASE/../nrf',
    )
    parser.add_argument(
        '--verbose', '-v', action='store_true', help='Show verbose output during checks'
    )
    parser.add_argument(
        '--year',
        '-y',
        type=int,
        nargs='*',
        help='Enable copyright year checking. \
                        If no years specified, checks current year. Can specify multiple years \
                        (e.g., --year 2023 2024 2025). Default: skip year checking',
    )
    parser.add_argument(
        '--config',
        '-c',
        type=str,
        help='Path to custom configuration YAML file. \
                        Default: matter_sample_checker_config.yaml in script directory',
    )
    parser.add_argument(
        '--allow-names',
        '-a',
        type=str,
        nargs='*',
        default=[],
        help='List of names/terms to allow during copy-paste error checking \
                        (case-insensitive). Use quotes for multi-word names.',
    )

    args = parser.parse_args()

    if not args.base:
        zephyr_base = os.environ.get("ZEPHYR_BASE")
        if not zephyr_base:
            print("Error: ZEPHYR_BASE environment variable is not set.")
            sys.exit(1)
        nrf_base = (Path(zephyr_base) / ".." / "nrf").resolve()
    else:
        nrf_base = Path(args.base)

    # Determine expected years based on --year argument
    expected_years = []
    if args.year is not None:  # --year was provided
        if len(args.year) == 0:  # --year with no arguments
            expected_years = [datetime.now().year]
        else:  # --year with specific years
            expected_years = args.year
    # If args.year is None (--year not provided), expected_years remains [] -> skip year checking

    # Determine which samples to check
    sample_paths = []

    if args.samples_zap_yaml:
        # Parse YAML file to get list of samples
        yaml_path = Path(args.samples_zap_yaml).resolve()
        if not yaml_path.exists():
            print(f"Error: YAML file does not exist: {yaml_path}")
            sys.exit(1)

        sample_paths = parse_samples_zap_yaml(yaml_path, nrf_base)

        if not sample_paths:
            print(f"Error: No valid samples found in YAML file: {yaml_path}")
            sys.exit(1)

        print(f"Found {len(sample_paths)} samples to check from {yaml_path}")
        if args.verbose:
            for sample in sample_paths:
                print(f"  - {sample}")
    else:
        # Single sample mode (original behavior)
        sample_path = Path(args.sample_path).resolve()
        if not sample_path.exists():
            print(f"Error: Sample path does not exist: {sample_path}")
            sys.exit(1)

        if not sample_path.is_dir():
            print(f"Error: Sample path is not a directory: {sample_path}")
            sys.exit(1)

        sample_paths = [sample_path]

    config_path = (
        args.config if args.config else Path(__file__).parent / 'matter_sample_checker_config.yaml'
    )

    # Load the configuration file
    config_dict = load_config(str(config_path))

    # Discover all checks in the checks directory
    checks_dir = Path(__file__).parent / 'checks'
    check_discovery = CheckDiscovery(checks_dir)
    check_classes = check_discovery.discover_checks()
    doc_check_classes = check_discovery.discover_doc_checks()

    if args.verbose:
        print(f"Discovered {len(check_classes)} sample checks:")
        for check_class in check_classes:
            print(f"  - {check_class.__name__}")
        print(f"Discovered {len(doc_check_classes)} documentation checks:")
        for check_class in doc_check_classes:
            print(f"  - {check_class.__name__}")

    # Run the checker on all samples
    all_reports = []
    total_issues = 0

    # Run documentation checks once (independent of samples)
    if doc_check_classes:
        if args.verbose:
            print(f"\nRunning {len(doc_check_classes)} documentation checks (once)...")

        checker = MatterSampleChecker(
            config_dict,
            nrf_base,
            verbose=args.verbose,
            allowed_names=args.allow_names,
            expected_years=expected_years,
            check_classes=doc_check_classes,
        )

        doc_report, doc_issues = checker.run_checks()

        if doc_report.strip():
            all_reports.append(doc_report)
            total_issues += doc_issues

    # Run sample-specific checks for each sample
    for sample_path in sample_paths:
        checker = MatterSampleChecker(
            config_dict,
            nrf_base,
            sample_path,
            verbose=args.verbose,
            allowed_names=args.allow_names,
            expected_years=expected_years,
            check_classes=check_classes,
        )

        report, issue_count = checker.run_checks()

        all_reports.append(report)
        total_issues += issue_count

    # Combine all reports
    final_report = "\n\n".join(all_reports)

    # Add summary for multi-sample mode
    if len(sample_paths) > 1:
        summary = f"\n{'=' * 80}\n"
        summary += "SUMMARY\n"
        summary += f"{'=' * 80}\n"
        summary += f"Total samples checked: {len(sample_paths)}\n"
        summary += f"Total issues found: {total_issues}\n"
        final_report = final_report + summary

    print(final_report)

    # Exit with error code if issues found
    return total_issues


if __name__ == '__main__':
    sys.exit(main())

#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import difflib
import re
from pathlib import Path
from typing import Any


def compare_overlay_files(
    current_file: Path, template_file: Path, config: dict[str, Any]
) -> list[str]:
    """Compare overlay files, checking that all required template lines exist in current file.

    This function:
    1. Gets all non-copyright, non-empty lines from the template file
    2. Checks if each template line exists anywhere in the sample file
    3. Preserves exact whitespace and indentation for comparison
    4. Allows sample file to have additional content not in template
    5. Returns a git-style unified diff when lines are missing
    """

    try:
        with open(current_file, encoding='utf-8') as f:
            current_lines = [line.rstrip() for line in f.readlines()]
    except Exception as e:
        return [f"Could not read current file {current_file.name}: {e}"]

    try:
        with open(template_file, encoding='utf-8') as f:
            template_lines = [line.rstrip() for line in f.readlines()]
    except Exception as e:
        return [f"Could not read template file {template_file.name}: {e}"]

    def is_copyright_line(line: str, copyright_pattern: str, copyright_keywords: list[str]) -> bool:
        """Check if a line contains copyright information."""
        if not line.strip():
            return False

        template_line_lower = line.strip().lower()

        # Check copyright pattern
        if re.search(copyright_pattern, line.strip(), re.IGNORECASE):
            return True

        # Check copyright keywords
        if any(keyword in template_line_lower for keyword in copyright_keywords):
            return True

        # Skip comment lines with copyright info or comment delimiters
        if line.strip().startswith(('*', '//', '/*', '#')):
            # Check for standalone comment delimiters (/* or */ or just *)
            stripped_line = line.strip()
            if stripped_line in ['/*', '*/', '*']:
                return True
            else:
                year_pattern = r'\b\d{4}\b'
                years_in_line = re.findall(year_pattern, line.strip())
                if years_in_line and all(2020 <= int(year) <= 2030 for year in years_in_line):
                    words_without_years = re.sub(year_pattern, '', line.strip()).strip()
                    if len(words_without_years) < 20 and any(
                        word in template_line_lower for word in ['nordic', 'copyright', '(c)']
                    ):
                        return True

        return False

    # Enhanced copyright keywords to filter out
    copyright_keywords = [
        'copyright',
        '(c)',
        'nordic semiconductor',
        'all rights reserved',
        'spdx-license-identifier',
        'license',
    ]

    copyright_pattern = config['license']['copyright_pattern']

    # Filter out copyright and empty lines from both files
    filtered_template_lines = []
    for line in template_lines:
        if line.strip() and not is_copyright_line(line, copyright_pattern, copyright_keywords):
            filtered_template_lines.append(line)

    filtered_current_lines = []
    for line in current_lines:
        if line.strip() and not is_copyright_line(line, copyright_pattern, copyright_keywords):
            filtered_current_lines.append(line)

    # Check each template line and its status in the current file
    current_content_set = set(filtered_current_lines)
    issues = []

    for line_num, template_line in enumerate(filtered_template_lines, 1):
        if template_line not in current_content_set:
            # Check if there's a similar line (for showing differences)
            similar_line = None
            template_stripped = template_line.strip()

            # Look for lines that might be similar (same content but different whitespace)
            for current_line in filtered_current_lines:
                current_stripped = current_line.strip()
                # Check if it's the same content but different formatting
                if template_stripped == current_stripped and template_line != current_line:
                    similar_line = current_line
                    break

            if similar_line:
                # Show the difference between template and sample versions
                issues.extend(
                    [
                        f"Line {line_num} differs:",
                        f"- Template: {repr(template_line)}",
                        f"+ Sample:   {repr(similar_line)}",
                    ]
                )
            else:
                # Line is completely missing
                issues.append(f"- Missing line {line_num}: {repr(template_line)}")

    # If there are issues, format them as a diff-style output
    if issues:
        diff_result = []
        diff_result.extend(issues)
        return diff_result

    return []


def compare_config_files(
    current_file: Path, template_file: Path, config: dict[str, Any]
) -> list[str]:
    """Compare two configuration files and return differences, excluding copyright lines."""
    # Use special overlay comparison for .overlay files
    if current_file.suffix == '.overlay':
        return compare_overlay_files(current_file, template_file, config)

    try:
        with open(current_file, encoding='utf-8') as f:
            current_content = f.readlines()
    except Exception as e:
        return [f"Could not read current file {current_file.name}: {e}"]

    try:
        with open(template_file, encoding='utf-8') as f:
            template_content = f.readlines()
    except Exception as e:
        return [f"Could not read template file {template_file.name}: {e}"]

    # Generate unified diff
    diff_lines = list(
        difflib.unified_diff(
            template_content,
            current_content,
            fromfile=f"template/{template_file.relative_to(template_file.parent.parent)}",
            tofile=f"current/{current_file.relative_to(current_file.parent.parent)}",
            lineterm='',
        )
    )

    # Return only the meaningful diff lines (skip header lines and context)
    if len(diff_lines) <= 3:  # Only has the header lines
        return []

    # Filter out copyright-related changes
    copyright_pattern = config['license']['copyright_pattern']
    filtered_diff_lines = []

    # Enhanced copyright keywords to filter out
    copyright_keywords = [
        'copyright',
        '(c)',
        'nordic semiconductor',
        'all rights reserved',
        'spdx-license-identifier',
        'license',
        '* Copyright',
        '// Copyright',
        '/* Copyright',
        ' * Copyright',
    ]

    for line in diff_lines:
        # Keep header lines and context lines
        if not line.startswith(('+', '-')) or line.startswith(('+++', '---')):
            filtered_diff_lines.append(line)
        else:
            # Skip lines that contain copyright information
            line_content = line[1:].strip()  # Remove the +/- prefix
            line_content_lower = line_content.lower()

            # Check for copyright pattern
            is_copyright_line = re.search(copyright_pattern, line_content, re.IGNORECASE)

            # Check for any copyright keywords
            if not is_copyright_line:
                is_copyright_line = any(
                    keyword in line_content_lower for keyword in copyright_keywords
                )

            # Also check for year-only changes in comment lines (common in copyright headers)
            if not is_copyright_line and (line_content.startswith(('*', '//', '/*', '#'))):
                # Check if line contains only year differences (4 consecutive digits)
                year_pattern = r'\b\d{4}\b'
                years_in_line = re.findall(year_pattern, line_content)
                if years_in_line and all(2020 <= int(year) <= 2030 for year in years_in_line):
                    # If the line is mostly just years and common words, it's likely a copyright
                    # year change
                    words_without_years = re.sub(year_pattern, '', line_content).strip()
                    if len(words_without_years) < 20 and any(
                        word in line_content_lower for word in ['nordic', 'copyright', '(c)']
                    ):
                        is_copyright_line = True

            # Only add the line if it's not copyright-related
            if not is_copyright_line:
                filtered_diff_lines.append(line)

    # Count actual changes after filtering (lines starting with + or -)
    change_lines = [
        line
        for line in filtered_diff_lines
        if line.startswith(('+', '-')) and not line.startswith(('+++', '---'))
    ]
    if not change_lines:
        return []

    return filtered_diff_lines

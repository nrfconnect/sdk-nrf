#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
 LICENSE YEAR VALIDATION CHECK:

Validates that all source files contain correct Nordic Semiconductor copyright
headers with the expected year. This ensures legal compliance and proper
attribution for all sample code files.

CONFIGURATION PARAMETERS:
-------------------------
• source_extensions: Array of file extensions to validate for copyright headers.
                    Only files with these extensions are checked for proper
                    Nordic Semiconductor copyright notices.

• copyright_pattern: Regular expression pattern to extract the copyright year
                    from file headers. Should capture the year as group 1.
                    Example pattern: "Copyright \\(c\\) (\\d{4})" matches
                    "Copyright (c) 2025" and captures "2025".

• disallowed_copyright_patterns: List of regex patterns for non-Nordic copyrights.
                                Files matching these patterns are silently skipped
                                (e.g., Matter SDK files, third-party libraries).

• exclude_dirs: Directories to exclude from license checking:
               - Build artifacts (build, build_debug, etc.)
               - Generated code (zap-generated, zcl_generated)
               - Third-party code (external libraries)

• license_exclude_patterns: File path patterns to exclude:
                           - Third-party code paths
                           - Generated files (*.zap, *.matter)
                           - External dependencies

VALIDATION STEPS:
-----------------
1. Source File Discovery:
   - Recursively scans sample directory for files matching source_extensions
   - Collects all files with specified extensions (.c, .cpp, .h, etc.)
   - Builds initial list of files to validate
   - Preserves relative paths for reporting

2. Directory Exclusion Filtering:
   - Applies exclude_dirs patterns to skip build and generated directories
   - Checks if any part of file path matches excluded directory names
   - Examples:
     * Files in "build/" are skipped
     * Files in "src/zap-generated/" are skipped
     * Files in "third_party/" are skipped
   - Reduces file list to only source files that need validation

3. Pattern Exclusion Filtering:
   - Applies license_exclude_patterns to file paths
   - Skips files matching excluded patterns (even if in valid directories)
   - Examples:
     * Files containing "/generated/" in path
     * Files with ".zap" extension
   - Final filtered list contains only files requiring Nordic copyright

4. Expected Year Validation:
   - Checks if expected_years parameter is provided
   - If no years specified, check is skipped
   - Typical years: [2024, 2025] for current and recent year
   - Years can be configured per release cycle

5. Copyright Header Reading:
   - Opens each file and reads first 100 characters
   - Copyright headers are typically at file beginning
   - Uses UTF-8 encoding for proper character handling
   - Handles read errors gracefully with warnings

6. Disallowed Pattern Detection:
   - Checks file header against disallowed_copyright_patterns
   - Silently skips files with non-Nordic copyrights:
     * Matter SDK files: "Copyright.*Project CHIP Authors"
     * CSA files: "Copyright.*Connectivity Standards Alliance"
     * Apache licensed: "Copyright.*Apache"
   - These files are legitimate but don't require Nordic copyright

7. Copyright Year Extraction:
   - Applies copyright_pattern regex to extract year
   - Captures year from patterns like:
     * "Copyright (c) 2025 Nordic Semiconductor ASA"
     * "Copyright (c) 2024 Nordic Semiconductor"
   - Converts captured year string to integer

8. Year Validation:
   - Compares extracted year against expected_years list
   - Valid if year matches any year in expected list
   - Invalid if year is outdated or missing

9. Issue Classification:
   - Files with correct year: Debug output (success)
   - Files with no copyright: Warning (should have copyright)
   - Files with wrong year: Error (needs update)

10. Results Reporting:
    - Debug output for files with correct copyright year
    - Warnings for files missing copyright headers
    - Errors for files with incorrect year
    - Shows expected years in error messages
    - Provides file paths relative to sample directory

EXPECTED YEARS CONFIGURATION:
-----------------------------
Expected years are typically passed via command-line or config:
• Current year: Always included (e.g., 2025)
• Previous year: Often included for recently updated files (e.g., 2024)
• Example: expected_years = [2024, 2025]

COMMON EXCLUSIONS:
------------------
Files that should NOT have Nordic copyright:
• Matter SDK generated files (zap-generated/)
• Third-party libraries (third_party/)
• Build artifacts (build/, build_*/
• External dependencies (imported from upstream)
• Auto-generated code (*.zap files, cluster XMLs)

NOTES:
------
• Only checks first 100 characters (copyright is always at top)
• Skips third-party and generated files automatically
• Case-sensitive pattern matching for copyright text
• Year comparison uses integer values (2024, 2025)
• Multiple years can be valid (allows for file creation vs. last update)
• Check is skipped if no expected years are configured
"""

import re

from internal.checker import MatterSampleTestCase


class LicenseYearTestCase(MatterSampleTestCase):
    def __init__(self):
        super().__init__()
        self.extensions = []
        self.exclude_dirs = []
        self.exclude_patterns = []
        self.disallowed_patterns = []
        self.copyright_pattern = ""
        self.expected_years = []
        self.source_files = []
        self.outdated_files = []
        self.current_year_files = []
        self.no_copyright_files = []

    def name(self) -> str:
        return "License Year"

    def prepare(self):
        self.expected_years = (
            self.config.expected_years if self.config.expected_years is not None else []
        )
        self.extensions = self.config.config_file.get('license').get('source_extensions')
        self.exclusions = self.config.config_file.get('exclusions')
        self.exclude_dirs = set(self.exclusions.get('exclude_dirs'))
        self.exclude_patterns = self.exclusions.get('license_exclude_patterns', [])

        for ext in self.extensions:
            for file_path in self.config.sample_path.rglob(f'*{ext}'):
                # Skip if file is in excluded directory
                if any(exclude_dir in file_path.parts for exclude_dir in self.exclude_dirs):
                    continue
                # Skip if file matches excluded patterns
                if any(pattern in str(file_path) for pattern in self.exclude_patterns):
                    continue
                self.source_files.append(file_path)

    def _should_skip(self, content):
        self.disallowed_patterns = self.config.config_file.get('license').get(
            'disallowed_copyright_patterns', []
        )

        return any(re.search(pattern, content) for pattern in self.disallowed_patterns)

    def _handle_file(self, file_path, content):
        # Returns True if file processed, False if skipped
        if self._should_skip(content):
            return False

        self.copyright_pattern = self.config.config_file.get('license').get('copyright_pattern')

        match = re.search(self.copyright_pattern, content)
        rel_path = file_path.relative_to(self.config.sample_path)
        if match:
            year = int(match.group(1))
            if year not in self.expected_years:
                self.outdated_files.append((rel_path, year))
            else:
                self.current_year_files.append(rel_path)
        else:
            self.no_copyright_files.append(rel_path)
        return True

    def check(self):
        if not self.expected_years:
            self.info("License year checking skipped (no years specified)")
            return

        for file_path in self.source_files:
            try:
                with open(file_path, encoding='utf-8') as f:
                    content = f.read(100)  # Check first 100 chars for license
                self._handle_file(file_path, content)
            except Exception as e:
                self.warning(
                    f"Could not read {file_path.relative_to(self.config.sample_path)}: {e}"
                )

        years_str = ', '.join(map(str, sorted(self.expected_years)))
        for file_path in self.current_year_files:
            self.debug(f"✓ Expected year(s) ({years_str}) license: {file_path}")

        for file_path in self.no_copyright_files:
            self.warning(f"No copyright found in: {file_path}")

        for file_path, year in self.outdated_files:
            self.issue(
                f"Incorrect copyright year in {file_path}: {year} (expected one of: {years_str})"
            )

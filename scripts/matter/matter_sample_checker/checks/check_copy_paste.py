#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
COPY-PASTE ERROR DETECTION CHECK:

Detects accidental copy-paste errors where a sample incorrectly references other
Matter samples in documentation or configuration files. This prevents publishing
samples with incorrect names, descriptions, or references left over from copying
another sample as a starting point.

VALIDATION STEPS:
-----------------
1. Sample Discovery:
   - Scans parent directory to discover all Matter samples
   - Extracts sample names from directory structure
   - Builds list of all samples in the Matter samples collection
   - Excludes current sample from forbidden words list

2. Forbidden Word Collection:
   - Collects names of all other samples as "forbidden words"
   - Each other sample's name should not appear in current sample
   - Filters out current sample name (self-references are allowed)
   - Creates comprehensive list of sample names to detect

3. Name Variant Generation:
   - Creates underscore variants: "window_covering" → "window covering"
   - Handles both directory names and user-friendly versions
   - Accounts for different naming conventions in documentation
   - Expands forbidden words list to catch all variations

4. Allowed Name Filtering:
   - Applies allowed_names filter from configuration
   - Some words may be sample names but also generic terms
   - Case-insensitive matching for allowed names
   - Removes explicitly allowed names from forbidden list

5. Minimum Length Filtering:
   - Removes sample names shorter than min_word_length
   - Prevents false positives from common short words
   - Configurable threshold (typically 4-5 characters)
   - Balances detection accuracy with false positive rate

6. File Content Scanning:
   - Checks each file in files_to_check list
   - Opens files with UTF-8 encoding
   - Splits content into individual lines for analysis
   - Tracks line numbers for precise error reporting

7. Pattern Matching:
   - Uses word-boundary regex to match whole words only
   - Case-insensitive matching to catch all variants
   - Prevents false positives from partial matches
   - Example: "lock" matches "lock" but not "block"

8. Skip Pattern Application:
   - Checks each matched line against skip_patterns
   - Allows legitimate references in documentation
   - Examples of skipped patterns:
     * "See also: :ref:`matter_lock_sample`" (RST reference)
     * "Based on the window_covering sample" (acknowledgment)
     * "Similar to light_bulb configuration" (comparison)

9. Severity Classification:
   - Non-README files: violations reported as errors (likely copy-paste)
   - README files: logged as DEBUG (not shown in normal output)
     * Can contain many legitimate references to other samples
     * Summary info message shows count if any potential issues found
     * Use --verbose flag to see all README references

10. Issue Reporting:
    - Reports filename, line number, and content preview
    - Shows found forbidden word in error message
    - Provides context (up to 100 characters of line content)
    - README files: single INFO summary + DEBUG details (with --verbose)

COMMON COPY-PASTE MISTAKES:
--------------------------
• Sample name in README.rst header
• Sample description in sample.yaml
• CMake project name from another sample
• File headers with incorrect sample references
• Documentation examples from another sample

NOTES:
------
• Only scans specific documentation and config files, not source code
• Case-insensitive matching catches all naming variations
• Word boundaries prevent false positives from partial matches
• README files: all references logged as DEBUG, with INFO summary if any found
  - Use --verbose to see all README references
  - Reduces noise since READMEs often legitimately reference other samples
• Non-README files: violations reported as errors (likely actual copy-paste)
• Template sample is included in forbidden words (catch copy-paste from template)
• Allowed names can be configured for terms that are both sample names and
  generic terms (e.g., "common", "utils")
• Space-separated variants automatically generated (e.g., "light bulb", "smoke co alarm")
"""

import re

from internal.checker import MatterSampleTestCase
from internal.utils.utils import discover_matter_samples


class CopyPasteTestCase(MatterSampleTestCase):
    def __init__(self):
        super().__init__()
        self.allowed_names = []
        self.forbidden_words = []
        self.all_samples = []
        self.files_to_check = []
        self.skip_patterns = []
        self.minimum_word_length = 0
        self.readme_issues_count = 0
        self.readme_found_words = set()

    def name(self) -> str:
        return "Copy Paste Errors"

    def prepare(self):
        # Names that can be excluded from the check
        self.allowed_names = self.config.allowed_names
        self.skip_names = self.config.config_file.get('copy_paste_detection').get('skip_names')
        self.all_samples = discover_matter_samples(self.config.sample_path)
        self.minimum_word_length = self.config.config_file.get('copy_paste_detection').get(
            'min_word_length'
        )
        self.files_to_check = self.config.config_file.get('copy_paste_detection').get(
            'files_to_check'
        )
        self.skip_patterns = self.config.config_file.get('copy_paste_detection').get(
            'skip_patterns'
        )

        self.discover_forbidden_words()

    def _should_skip_line(self, line_lower, word, file_path):
        # Skip defined patterns and allowed names for the specific sample
        if any(pattern in line_lower for pattern in self.skip_patterns) or (
            self.sample_name in self.skip_names and word in self.skip_names[self.sample_name]
        ):
            return True

    def _handle_readme_issue(self, file_path, i, word, line):
        # Debug log and count/report unique word
        self.debug(
            f"Potential copy-paste in {file_path}:{i} - found'{word}' in: {line.strip()[:100]}"
        )
        self.readme_issues_count += 1
        self.readme_found_words.add(word)

    def _handle_non_readme_issue(self, file_path, i, word, line):
        self.issue(
            f"Possible copy-paste error in {file_path}:{i} - found'{word}' in: {line.strip()[:100]}"
        )

    def check(self):
        self.debug(f"Checking for references to other samples: {', '.join(self.forbidden_words)}")

        for file_path in self.files_to_check:
            full_path = self.config.sample_path / file_path
            if not full_path.exists():
                continue
            try:
                with open(full_path, encoding='utf-8') as f:
                    content = f.read()
            except Exception as e:
                self.warning(f"Could not check {file_path} for copy-paste errors: {e}")
                continue

            for word in self.forbidden_words:
                word_pattern = r'\b' + re.escape(word) + r'\b'
                if not re.search(word_pattern, content, re.IGNORECASE):
                    continue

                lines = content.split('\n')
                is_readme = file_path in ['README.rst', 'README.md']

                # To avoid deep nesting, flatten loop for each word
                for i, line in enumerate(lines, 1):
                    if not re.search(word_pattern, line, re.IGNORECASE):
                        continue
                    line_lower = line.lower().strip()
                    if self._should_skip_line(line_lower, word, file_path):
                        continue
                    if is_readme:
                        self._handle_readme_issue(file_path, i, word, line)
                    else:
                        self._handle_non_readme_issue(file_path, i, word, line)

        # After checking all files, if there were README issues, show summary
        if self.readme_issues_count > 0:
            found_words_list = sorted(self.readme_found_words)
            self.info(
                f"Found {self.readme_issues_count} potential copy-paste mistake(s) in the "
                "README.rst file. There are references to the following words:"
            )
            for word in found_words_list:
                self.info(f"  - {word}")
            self.info("Run the script again with --verbose to see details.")

    def discover_forbidden_words(self):
        """Discover all forbidden words from all samples."""
        for sample in self.all_samples:
            if sample != self.sample_name:
                self.forbidden_words.append(sample)

        # Do not split names into parts; only use the full sample name as forbidden
        # Remove duplicates and very short words
        forbidden_words = [
            word for word in self.forbidden_words if len(word) >= self.minimum_word_length
        ]

        # Also add forbidden words with underscores replaced by spaces
        # (for copy-paste errors in documentation)
        forbidden_words_with_spaces = []
        for word in forbidden_words:
            if '_' in word:
                forbidden_words_with_spaces.append(word.replace('_', ' '))
        forbidden_words.extend(forbidden_words_with_spaces)

        # Filter out allowed names (case-insensitive matching)
        if self.allowed_names:
            allowed_names_lower = {name.lower() for name in self.allowed_names}
            original_count = len(forbidden_words)
            forbidden_words = [
                word for word in forbidden_words if word.lower() not in allowed_names_lower
            ]
            filtered_count = original_count - len(forbidden_words)
            if filtered_count > 0:
                self.debug(
                    f"Filtered out {filtered_count} allowed name(s): "
                    f"{', '.join(sorted(self.allowed_names))}"
                )

        # Update the instance variable with all the processed forbidden words
        self.forbidden_words = forbidden_words

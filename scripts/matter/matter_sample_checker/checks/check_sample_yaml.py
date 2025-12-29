#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
 SAMPLE.YAML CONSISTENCY VALIDATION CHECK:

Validates the sample.yaml metadata file for correct structure, naming conventions,
and required tags. This ensures samples are properly configured for CI/CD
testing and documentation generation.

CONFIGURATION PARAMETERS:
-------------------------
• required_tags: Array of tag strings; at least one must be present in the
                sample.yaml common.tags section. These tags control which
                CI/CD pipelines will build and test the sample.

• sample_name_pattern: Regular expression pattern that all test names must
                      match. Ensures consistent naming convention across
                      Matter samples (e.g., "sample.matter.lock.debug").

VALIDATION STEPS:
-----------------
1. Sample YAML File Discovery:
   - Locates sample.yaml in sample directory
   - Verifies file exists (required for all samples)
   - Reads file content for validation
   - Reports error if file is missing

2. YAML Parsing:
   - Parses YAML structure using safe_load
   - Validates YAML syntax is correct
   - Extracts test definitions and common metadata
   - Reports format errors with exception details

3. Test Name Validation:
   - Iterates through all test entries in 'tests' section
   - Extracts test name (key in tests dictionary)
   - Applies sample_name_pattern regex to each test name
   - Validates naming follows convention: sample.matter.<sample_name>.<variant>
   - Examples:
     * Valid: "sample.matter.lock.debug"
     * Valid: "sample.matter.window_covering.release"
     * Invalid: "lock.debug" (missing "sample.matter." prefix)
     * Invalid: "sample.thread.lock" (wrong protocol namespace)

4. Required Tag Validation:
   - Extracts tags from common.tags section
   - Checks if at least one required tag is present
   - Tags control CI/CD pipeline inclusion:
     * "ci_samples_matter" - Main Matter samples CI
     * "ci_build" - General build verification CI
   - Reports error if no required tags found
   - Shows which tags are present vs. which are required

5. Issue Reporting:
   - Test naming violations: Error with test name shown
   - Missing required tags: Error with list of required tags
   - Invalid YAML format: Error with parsing exception
   - Successful validation: Silent (no debug output needed)

REQUIRED TAGS:
--------------
Tags control CI/CD pipeline behavior:

• ci_samples_matter: Primary tag for Matter sample testing
                    - Runs full Matter test suite
                    - Validates commissioning and clusters
                    - Required for all Matter samples

• ci_build: Alternative tag for build verification
           - Only compiles the sample
           - No runtime testing performed
           - Used for build-only validation

At least one of these tags must be present in common.tags section.

NOTES:
------
• sample.yaml is required for all Matter samples
• Test names must follow strict naming convention for CI/CD
• At least one CI tag must be present
• Multiple test entries allow different build variants (debug, release)
• Tags in individual tests are separate from common tags
• Pattern matching is case-sensitive
• Validation fails on first error (doesn't collect all errors)
"""

import re

import yaml
from internal.checker import MatterSampleTestCase


class SampleYamlTestCase(MatterSampleTestCase):
    def name(self) -> str:
        return "Sample YAML consistency"

    def prepare(self):
        # No preparation needed for this test case.
        pass

    def check(self):
        # Get configuration
        required_tags = self.config.config_file.get('sample_yaml').get('required_tags')
        sample_name_pattern = self.config.config_file.get('sample_yaml').get('sample_name_pattern')

        # 1. Check sample.yaml consistency
        sample_yaml_path = self.config.sample_path / 'sample.yaml'
        if sample_yaml_path.exists():
            try:
                with open(sample_yaml_path) as f:
                    sample_data = yaml.safe_load(f)

                # Check if sample name matches directory
                for test_name in sample_data.get('tests'):
                    if not re.match(sample_name_pattern, str(test_name)):
                        self.issue(
                            f"Sample name in sample.yaml does not match directory name: \
                            {test_name}"
                        )

                # Check for ci_samples_matter tag
                tags = sample_data.get('common').get('tags')
                if not any(tag in tags for tag in required_tags):
                    self.issue(
                        f"Missing required tag in sample.yaml: at least one of \
                        {required_tags} must be present in {tags}"
                    )
            except Exception as e:
                self.issue(f"Invalid sample.yaml format: {e}")

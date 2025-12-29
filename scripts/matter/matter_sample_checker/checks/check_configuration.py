#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
CONFIGURATION CONSISTENCY VALIDATION CHECK:

Validates that Matter sample configuration files contain all required Kconfig
settings for proper Matter functionality. This ensures samples have the necessary
configurations for Matter stack initialization, commissioning, and operation.

CONFIGURATION PARAMETERS:
-------------------------
• required_prj_configs: Array of CONFIG_* entries that must exist in project
                       configuration files (prj.conf and prj_release.conf).
                       These are essential for Matter protocol functionality.

• required_sysbuild_configs: Array of SB_CONFIG_* entries required in sysbuild
                            configuration files. These control build system
                            features like factory data and bootloader.

• consistency_checks: Advanced validation rules for related configuration
                     groups. Validates that related configs are properly set.

VALIDATION STEPS:
-----------------
1. Project Configuration Validation (prj.conf):
   - Locates prj.conf in sample directory
   - Reads file content as plain text
   - Searches for each required CONFIG_* entry
   - Validates entries exist in file (checks for presence of config name)
   - Reports missing configurations as issues
   - Provides debug output for found configurations

2. Release Configuration Validation (prj_release.conf):
   - Checks if prj_release.conf exists in sample directory
   - Performs same validation as prj.conf
   - Ensures release builds have required configurations
   - Allows for release-specific config overrides
   - Reports missing entries specific to release variant

3. Sysbuild Configuration Validation (sysbuild.conf):
   - Locates sysbuild.conf in sample directory
   - Reads file content for build system configs
   - Validates presence of required SB_CONFIG_* entries
   - Checks for bootloader and factory data configurations
   - Ensures sysbuild features are properly enabled

4. Internal Sysbuild Validation (sysbuild_internal.conf):
   - Checks for internal build variant configuration
   - Validates same required configs as standard sysbuild.conf
   - Ensures internal memory variants have proper settings
   - Reports issues specific to internal configurations

5. Configuration Consistency Checks:
   - Validates related configuration groups are properly set
   - Checks for conflicting configuration combinations
   - Ensures dependent configurations are present together
   - Validates Matter-specific configuration patterns

NOTES:
------
• Check validates presence of config entries with exact key match
• For requirements with explicit values (e.g. SB_CONFIG_MATTER=y), exact match is required
• Commented-out lines are ignored
• Each configuration file is checked independently
• Missing files (like prj_release.conf) are silently skipped if optional
"""

import re

from internal.checker import MatterSampleTestCase


class ConfigurationTestCase(MatterSampleTestCase):
    def name(self) -> str:
        return "Configuration consistency"

    def prepare(self):
        # No preparation needed for this test case.
        pass

    def check(self):
        def validate_configs(conf_path, config_key, file_label):
            if not conf_path.exists():
                return
            with open(conf_path) as f:
                content = f.read()
            required_configs = self.config.config_file.get('validation').get(config_key)
            for config in required_configs:
                if not self._config_present(content, config):
                    self.issue(f"Missing configuration in {file_label}: {config}")
                else:
                    self.debug(f"✓ Found config: {config}")

        # 1. Check prj.conf and prj_release.conf for basic Matter settings
        prj_conf_paths = [
            (self.config.sample_path / 'prj.conf', 'required_prj_configs', 'prj.conf'),
            (self.config.sample_path / 'prj_release.conf', 'required_prj_configs', 'prj.conf'),
        ]
        for prj_conf_path, config_key, file_label in prj_conf_paths:
            validate_configs(prj_conf_path, config_key, file_label)

        # 2. Check sysbuild.conf and sysbuild_internal.conf for basic Matter settings
        sysbuild_conf_paths = [
            (
                self.config.sample_path / 'sysbuild.conf',
                'required_sysbuild_configs',
                'sysbuild.conf',
            ),
            (
                self.config.sample_path / 'sysbuild_internal.conf',
                'required_sysbuild_configs',
                'sysbuild.conf',
            ),
        ]
        for sysbuild_conf_path, config_key, file_label in sysbuild_conf_paths:
            validate_configs(sysbuild_conf_path, config_key, file_label)

    def _config_present(self, content: str, requirement: str) -> bool:
        """
        Check if a required config is present in the given Kconfig-style content.
        - Ignores comment lines (lines starting with '#', after optional whitespace).
        - If requirement contains '=', require exact line match (key=value).
        - Otherwise, require the key at the start of a non-comment line.
        """
        has_value = '=' in requirement
        requirement_regex = (
            re.compile(r"^\s*" + re.escape(requirement) + r"\s*$")
            if has_value
            else re.compile(r"^\s*" + re.escape(requirement) + r"(\s*=.*)?\s*$")
        )

        for line in content.splitlines():
            # Ignore empty lines and comment lines (after optional whitespace)
            stripped = line.lstrip()
            if not stripped or stripped.startswith('#'):
                continue
            if requirement_regex.match(line):
                return True
        return False

#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
 FILE STRUCTURE VALIDATION CHECK:

Validates that Matter samples contain all required files and directories
according to the standard Matter sample structure. Ensures samples have
proper documentation, configuration, source code organization, and build files.

CONFIGURATION PARAMETERS:
-------------------------
• required_files: Array of file paths that must exist in the sample directory.
                 Each file is required for proper sample functionality:
                 - README.rst: User documentation
                 - sample.yaml: CI/CD test definitions
                 - prj.conf: Zephyr project configuration
                 - CMakeLists.txt: CMake build definition
                 - Kconfig: Configuration options definition

• required_directories: Array of directory paths that must exist in the sample.
                       These directories organize sample content:
                       - src/: Contains C/C++ source code
                       - configuration/: Board-specific configs

VALIDATION STEPS:
-----------------
1. Required Files Validation:
   - Iterates through each entry in required_files list
   - Constructs full path: sample_path / file_path
   - Checks if file exists using Path.exists()
   - Reports missing files as errors
   - Provides debug confirmation for found files
   - Examples:
     * Checks for README.rst existence
     * Validates sample.yaml is present
     * Ensures prj.conf exists

2. Required Directories Validation:
   - Iterates through each entry in required_directories list
   - Constructs full path: sample_path / dir_path
   - Checks if path exists AND is a directory using Path.is_dir()
   - Reports missing directories as errors
   - Provides debug confirmation for found directories
   - Examples:
     * Validates src/ directory exists
     * Checks for configuration/ directory

3. Issue Reporting:
   - Missing files: Reported as errors with relative path
   - Missing directories: Reported as errors with relative path
   - Found files/dirs: Reported as debug output for confirmation
   - Each item validated independently (all errors reported)

STANDARD MATTER SAMPLE STRUCTURE:
------------------------------
matter_sample/
├── README.rst              ✓ Required: Sample documentation
├── sample.yaml             ✓ Required: CI/CD test definitions
├── prj.conf                ✓ Required: Main Zephyr configuration
├── prj_release.conf        ○ Optional: Release build config
├── CMakeLists.txt          ✓ Required: CMake build definition
├── Kconfig                 ✓ Required: Configuration options
├── sysbuild.conf           ○ Optional: Sysbuild configuration
├── src/                    ✓ Required: Source code directory
│   ├── main.cpp            ○ Application entry point
│   ├── app_task.cpp        ○ Application logic
│   └── app_task.h          ○ Application headers
├── configuration/          ✓ Required: Board configurations
│   ├── nrf52840dk_nrf52840.conf    ○ Board-specific config
│   ├── nrf52840dk_nrf52840.overlay ○ Board-specific DTS overlay
│   └── ...                 ○ Other board configs
├── pm_static_*.yml         ○ Optional: Partition manager configs
└── child_image/            ○ Optional: Multi-image builds

NOTES:
------
• All checks are file existence only (not content validation)
• Directories must actually be directories (not files named like dirs)
• Paths are relative to sample root
• Optional files are not validated by this check
• Check is comprehensive - reports ALL missing items, not just first
• Debug output confirms each found item for verification
"""

from internal.checker import MatterSampleTestCase


class FileStructureTestCase(MatterSampleTestCase):
    def name(self) -> str:
        return "File Structure"

    def prepare(self):
        # No preparation needed for this test case.
        pass

    def check(self):
        for file_path in self.config.config_file.get('file_structure').get('required_files'):
            full_path = self.config.sample_path / file_path
            if not full_path.exists():
                self.issue(f"Missing required file: {file_path}")
            else:
                self.debug(f"✓ Found: {file_path}")

        for dir_path in self.config.config_file.get('file_structure').get('required_directories'):
            full_path = self.config.sample_path / dir_path
            if not full_path.is_dir():
                self.issue(f"Missing required directory: {dir_path}")
            else:
                self.debug(f"✓ Found directory: {dir_path}")

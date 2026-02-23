#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
HARDWARE REQUIREMENTS DOCUMENTATION VALIDATION CHECK:

Validates the consistency and accuracy of hardware requirements documentation,
including build configurations, diagnostic logs, and memory layouts. This check
ensures that documentation matches the actual implementation in sample.yaml files
and pm_static files.

CONFIGURATION PARAMETERS:
-------------------------
• documentation.requirements.path: Path to the hw_requirements.rst file relative
                                   to the nRF repository root.

VALIDATION STEPS:
-----------------
1. Build Configuration Validation:
   - Analyzes sample.yaml files for all Matter samples and applications
   - Extracts and validates build types (debug, release, internal, wifi_thread_switched)
   - Verifies that critical samples have required build configurations

2. Diagnostic Logs Validation:
   - Reads DTS overlay files from matter-diagnostic-logs snippet
   - Parses retention partition definitions and sizes
   - Compares partition sizes with documentation tables


NOTES:
------
• This check runs once per test session, not per sample
• Validates consistency between multiple sources: samples, snippets, and docs
• Handles board name mappings between different naming conventions
"""

import re
from pathlib import Path

import yaml
from internal.checker import MatterSampleTestCase

# Bidirectional board mapping (board_id <-> display_name)
BOARD_MAPPINGS = {
    'nrf52840dk_nrf52840': 'nRF52840 DK',
    'nrf5340dk_nrf5340_cpuapp': 'nRF5340 DK',
    'nrf7002dk_nrf5340_cpuapp': 'nRF7002 DK',
    'thingy53_nrf5340_cpuapp': 'Nordic Thingy:53',
    'nrf54l15dk_nrf54l15_cpuapp': 'nRF54L15 DK',
    'nrf54l15dk_nrf54l10_cpuapp': 'nRF54L10 emulation on nRF54L15 DK',
    'nrf54lm20dk_nrf54lm20a_cpuapp': 'nRF54LM20 DK',
    'nrf54l15dk_nrf54l15_cpuapp_internal': 'nRF54L15 DK with internal memory only',
    'nrf54l15dk_nrf54l15_cpuapp_ns': 'nRF54L15 DK with TF-M',
    'nrf54lm20dk_nrf54lm20a_cpuapp_internal': 'nRF54LM20 DK with internal memory only',
}

# Helper functions for board mapping


def get_display_name(board_id: str) -> str:
    """Get display name from board ID"""
    return BOARD_MAPPINGS.get(board_id, board_id)


# Partition configuration
PARTITION_CONFIG = {
    'retention': {
        'crash_retention': 'Crash retention',
        'network_logs_retention': 'Network logs retention',
        'end_user_logs_retention': 'User data logs retention',
    },
}


class CheckDocRequirementsTestCase(MatterSampleTestCase):
    """Check hardware requirements documentation consistency"""

    def __init__(self):
        super().__init__()
        self.file_path = None
        self.content = ""
        self._requirements_cache = None  # Cache for parsed requirements data

    def name(self) -> str:
        return "Hardware requirements documentation check"

    def prepare(self):
        """Prepare the check by reading the requirements file"""
        self.file_path = (
            self.config.config_file.get("documentation").get("requirements").get("path")
        )
        try:
            with open(self.config.nrf_path / self.file_path) as f:
                self.content = f.read()
        except Exception as e:
            self.issue(f"Error reading file: {e}")

    def check(self):
        """Run the comprehensive hardware requirements validation"""
        if not self.content:
            self.issue("No content to validate")
            return

        # Basic content analysis
        tables_count = self.content.count('+-') // 2
        tabs_count = self.content.count('.. tab::')
        self.debug(f"Found {tables_count} tables and {tabs_count} tabs")

        # Get sample and application information
        samples_path = self.config.nrf_path / "samples" / "matter"
        apps_path = self.config.nrf_path / "applications"

        samples_info = self._analyze_matter_samples(samples_path)
        apps_info = self._analyze_matter_applications(apps_path)

        # Validate build configurations
        self._validate_build_configurations(samples_info, apps_info)

        # Validate diagnostic logs
        self._validate_diagnostic_logs()

    def _analyze_matter_samples(self, samples_path: Path) -> dict:
        """Quick analysis of Matter samples"""
        samples_info = {}

        if not samples_path.exists():
            return samples_info

        for sample_dir in samples_path.iterdir():
            if not sample_dir.is_dir():
                continue

            sample_yaml = sample_dir / "sample.yaml"
            if not sample_yaml.exists():
                continue

            try:
                with open(sample_yaml) as f:
                    config = yaml.safe_load(f)

                # Extract build types
                build_types = set()

                if 'tests' in config:
                    for test_name in config['tests']:
                        build_type = self._extract_build_type_from_name(test_name)
                        if build_type:
                            build_types.add(build_type)

                samples_info[sample_dir.name] = {'build_types': build_types}

            except Exception:
                continue  # Skip problematic samples

        return samples_info

    def _analyze_matter_applications(self, apps_path: Path) -> dict:
        """Quick analysis of Matter applications"""
        apps_info = {}
        app_names = ['matter_weather_station', 'matter_bridge']

        for app_name in app_names:
            app_path = apps_path / app_name
            sample_yaml = app_path / "sample.yaml"

            if not sample_yaml.exists():
                continue

            try:
                with open(sample_yaml) as f:
                    config = yaml.safe_load(f)

                build_types = set()
                if 'tests' in config:
                    for test_name in config['tests']:
                        build_type = self._extract_build_type_from_name(test_name)
                        if build_type:
                            build_types.add(build_type)

                apps_info[app_name] = {'build_types': build_types}

            except Exception:
                continue

        return apps_info

    def _extract_build_type_from_name(self, test_name: str) -> str:
        """Extract build type from test name"""
        test_name_lower = test_name.lower()

        if 'thread_wifi_switched' in test_name_lower or 'wifi_thread_switched' in test_name_lower:
            return 'wifi_thread_switched'
        elif '.release' in test_name_lower:
            if 'internal' in test_name_lower:
                return 'internal'
            return 'release'
        elif '.debug' in test_name_lower:
            return 'debug'

        return ""

    def _validate_build_configurations(self, samples: dict, apps: dict) -> None:
        """Validate build configurations with detailed tracking"""
        all_projects = {**samples, **apps}

        for project_name, project_info in all_projects.items():
            available_builds = project_info.get('build_types', set())

            # Check for missing critical build types
            if project_name == 'template':
                has_internal = 'internal' in available_builds
                if not has_internal:
                    self.issue("Template sample missing 'internal' build type")

            if project_name == 'lock':
                has_wifi_switched = 'wifi_thread_switched' in available_builds
                if not has_wifi_switched:
                    self.warning("Lock sample should support 'wifi_thread_switched'")

            # Check for missing debug/release
            basic_builds = {'debug', 'release'}
            missing_basic = basic_builds - available_builds
            if missing_basic and project_name != 'manufacturer_specific':
                for build in missing_basic:
                    self.warning(f"Sample '{project_name}' missing '{build}' build")

    def _validate_diagnostic_logs(self) -> None:
        """
        Enhanced validation of diagnostic logs - reads DTS overlays and compares retention
        partitions.
        """
        diag_logs_path = (
            self.config.nrf_path / "snippets" / "matter" / "matter-diagnostic-logs" / "boards"
        )

        if not diag_logs_path.exists():
            self.issue("Diagnostic logs snippet directory not found")
            return

        # Check if diagnostic logs section exists
        has_diag_section = 'Diagnostic logs RAM memory requirements' in self.content
        if not has_diag_section:
            self.warning("Diagnostic logs section not found in documentation")

        # Get overlay files and analyze retention partitions
        overlay_files = list(diag_logs_path.glob("*.overlay"))
        overlay_count = len(overlay_files)

        if self.config.verbose:
            self.debug(
                f"\n+++ Analyzing {overlay_count} DTS overlay files for retention partitions"
            )

        # Analyze each overlay file for retention partitions
        self._validate_retention_partitions(overlay_files)

    def _validate_retention_partitions(self, overlay_files: list[Path]) -> None:
        """Validate retention partitions in DTS overlay files against sample configurations"""
        for overlay_file in overlay_files:
            board_name = self._extract_board_name_from_overlay(overlay_file.name)
            if self.config.verbose:
                self.debug(f"+ Analyzing {overlay_file.name} for board {board_name}:")

            try:
                with open(overlay_file) as f:
                    dts_content = f.read()

                # Extract retention partition information
                partitions = self._parse_retention_partitions_from_dts(dts_content)

                # Validate expected partitions are present
                for partition_key, partition_name in PARTITION_CONFIG['retention'].items():
                    if partition_key not in partitions:
                        self.warning(
                            f"Board {board_name}: Missing {partition_name} partition in overlay"
                        )
                    else:
                        partition_info = partitions[partition_key]
                        if self.config.verbose:
                            self.debug(
                                f"\tFound {partition_name}: "
                                f"offset=0x{partition_info['offset']:x}, "
                                f"size=0x{partition_info['size']:x} \
                                       ({partition_info['size']} bytes)"
                            )

                self._validate_partition_sizes(partitions, board_name)

            except Exception as e:
                self.issue(f"Error reading DTS overlay {overlay_file.name}: {e}")

    def _extract_board_name_from_overlay(self, filename: str) -> str:
        """Extract board name from overlay filename"""
        # Remove .overlay extension to get the full board identifier
        board_name = filename.replace('.overlay', '')
        return board_name

    def _parse_retention_partitions_from_dts(self, dts_content: str) -> dict:
        """Parse retention partition information from DTS content"""
        partitions = {}

        # Pattern to match retention partition declarations
        # Example: crash_retention: retention@20070000 { reg = <0x20070000 0x10000>; };
        retention_pattern = (
            r'(\w+):\s*retention@([0-9a-fA-F]+)\s*\{[^}]*?reg\s*=\s*<\s*'
            r'0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s*>\s*;[^}]*\}'
        )

        matches = re.findall(retention_pattern, dts_content, re.MULTILINE | re.DOTALL)

        for match in matches:
            partition_name, _, reg_offset, reg_size = match

            # Convert hex strings to integers
            offset = int(reg_offset, 16)
            size = int(reg_size, 16)

            # Map common partition names to standardized keys
            partition_key = self._map_partition_name_to_key(partition_name)

            partitions[partition_key] = {
                'name': partition_name,
                'offset': offset,
                'size': size,
            }

        return partitions

    def _map_partition_name_to_key(self, partition_name: str) -> str:
        """Map DTS and requirements partition names to standardized keys"""
        name_mapping = {
            # DTS partition names
            'crash_retention': 'crash_retention',
            'network_logs_retention': 'network_logs_retention',
            'end_user_logs_retention': 'end_user_logs_retention',
        }

        return name_mapping.get(partition_name.lower(), partition_name.lower())

    def _parse_requirements_partition_tables(
        self, requirements_file_path: str
    ) -> dict[str, dict[str, dict]]:
        """Parse board-specific partition tables from requirements.rst file"""
        if self._requirements_cache is not None:
            return self._requirements_cache
        board_partition_data = {}
        try:
            with open(requirements_file_path, encoding='utf-8') as f:
                content = f.read()
            lines = content.split('\n')
            start_idx = self._find_diag_section_start(lines)
            if start_idx == -1:
                return board_partition_data
            end_idx = self._find_diag_section_end(lines, start_idx)
            diagnostic_content = '\n'.join(lines[start_idx:end_idx])
            tab_pattern = r'.. tab:: (.+?)\n(.*?)(?=.. tab:|$)'
            tab_matches = re.findall(tab_pattern, diagnostic_content, re.DOTALL)
            for board_name, tab_content in tab_matches:
                board_name = board_name.strip()
                partitions = self._extract_partitions_from_tab(tab_content, board_name)
                if partitions:
                    board_partition_data[board_name] = partitions
        except Exception as e:
            self.warning(f"Error parsing requirements file: {e}")
        self._requirements_cache = board_partition_data
        return board_partition_data

    def _find_diag_section_start(self, lines):
        for i, line in enumerate(lines):
            if 'Diagnostic logs RAM memory requirements' in line:
                return i
        return -1

    def _find_diag_section_end(self, lines, start_idx):
        end_idx = len(lines)
        for i in range(start_idx + 2, len(lines)):
            if (
                lines[i].strip()
                and not lines[i].startswith((' ', '\t', '..', '+', '|'))
                and i + 1 < len(lines)
                and lines[i + 1].strip()
                and lines[i + 1][0] in '=-^~'
            ):
                return i
        return end_idx

    def _extract_partitions_from_tab(self, tab_content, board_name):
        partitions = {}
        tab_lines = tab_content.split('\n')
        rows = self._extract_table_rows(tab_lines)
        if not rows:
            return partitions
        for row in rows[1:]:  # Skip header row
            partition_name, offset_str, size_str = row[0].strip(), row[1].strip(), row[2].strip()
            if 'retention' in partition_name.lower():
                size_data = self._parse_value_from_string(size_str, allow_none=False)
                offset_data = self._parse_value_from_string(offset_str, allow_none=True)
                self._check_size_consistency(size_data, board_name, partition_name)
                self._check_offset_consistency(offset_data, board_name, partition_name)
                if size_data['bytes'] is not None:
                    partition_key = self._map_partition_name_to_key(partition_name)
                    partitions[partition_key] = {
                        'name': partition_name,
                        'size': size_data['bytes'],
                        'offset': offset_data['bytes'] if offset_data['bytes'] is not None else 0,
                        'size_hex': f"0x{size_data['hex_value']:X}"
                        if size_data['hex_value']
                        else '0x0',
                        'offset_hex': f"0x{offset_data['hex_value']:X}"
                        if offset_data['hex_value']
                        else '0x0',
                    }
        return partitions

    def _extract_table_rows(self, tab_lines):
        rows = []
        for line in tab_lines:
            if '|' in line and not line.strip().startswith(('+', '=')):
                columns = [col.strip() for col in line.split('|')[1:-1]]
                if len(columns) == 3 and columns[0]:
                    rows.append(columns)
        return rows

    def _check_size_consistency(self, size_data, board_name, partition_name):
        if size_data['is_consistent']:
            return
        multiplier = self._unit_multiplier(size_data['unit_text'])
        unit_bytes = int(size_data['unit_value'] * multiplier)
        hex_value_kb = size_data['hex_value'] / 1024 if size_data['hex_value'] else 0
        self.issue(
            f"{board_name}: {partition_name} SIZE MISMATCH - "
            f"Table decimal: \033[91m{hex_value_kb:.2f} KB "
            f"(0x{size_data['hex_value']:X})\033[0m "
            f"vs dts: \033[91m{size_data['unit_value']} {size_data['unit_text']} "
            f"(0x{unit_bytes:X})\033[0m"
        )

    def _check_offset_consistency(self, offset_data, board_name, partition_name):
        if offset_data['is_consistent']:
            return
        multiplier = self._unit_multiplier(offset_data['unit_text'])
        unit_bytes = int(offset_data['unit_value'] * multiplier)
        hex_value_kb = offset_data['hex_value'] / 1024 if offset_data['hex_value'] else 0
        self.issue(
            f"{board_name}: {partition_name} OFFSET MISMATCH - "
            f"Table decimal: \033[91m{offset_data['unit_value']} "
            f"{offset_data['unit_text']} (0x{unit_bytes:X})\033[0m "
            f"vs dts: \033[91m{hex_value_kb:.2f} KB "
            f"(0x{offset_data['hex_value']:X})\033[0m"
        )

    def _unit_multiplier(self, unit_text):
        if unit_text in ['KB', 'kB']:
            return 1024
        if unit_text == 'MB':
            return 1024 * 1024
        return 1

    def _get_board_requirements_data(
        self, board_name: str, requirements_file_path: str
    ) -> dict[str, dict]:
        """Get partition requirements for a specific board"""
        # Parse requirements data
        requirements_data = self._parse_requirements_partition_tables(requirements_file_path)

        # Find the correct board name in requirements
        req_board_name = get_display_name(board_name)

        return requirements_data.get(req_board_name, {})

    def _validate_partition_sizes(self, partitions: dict, board_name: str) -> None:
        """Validate that partition sizes match requirements from documentation"""
        requirements_file_path = str(self.config.nrf_path / self.file_path)

        # Get requirements data for this board
        board_requirements = self._get_board_requirements_data(board_name, requirements_file_path)

        if not board_requirements:
            self.warning(f"Board {board_name}: No partition requirements found in documentation")
            return

        # Create a mapping between DTS partition names and requirements partition names
        # Requirements uses spaces, DTS uses underscores
        dts_to_req_mapping = {
            'crash_retention': 'crash retention',
            'network_logs_retention': 'network logs retention',
            'end_user_logs_retention': 'user data logs retention',
        }

        # Check for missing partitions
        req_to_dts_mapping = {v: k for k, v in dts_to_req_mapping.items()}
        for req_partition_key, req_partition_info in board_requirements.items():
            # Map requirements partition name back to DTS name
            dts_partition_key = req_to_dts_mapping.get(req_partition_key, req_partition_key)
            if dts_partition_key not in partitions:
                self.warning(
                    f"Board {board_name}: Required partition \
                    '{req_partition_info['name']}' not found in DTS file"
                )

    @staticmethod
    def _parse_value_from_string(value_str: str, allow_none: bool = False) -> dict:
        """
        Parse size/offset values from strings like '28 kB (0x7000)' or '960 kB (0xf0000)'
        Returns a dict with 'bytes', 'hex_value', 'unit_value', 'unit_text', and 'is_consistent'
        """
        if not value_str or value_str.strip() == '':
            return {
                'bytes': None if allow_none else 0,
                'hex_value': None,
                'unit_value': None,
                'unit_text': None,
                'is_consistent': True,
            }

        result = {
            'bytes': None if allow_none else 0,
            'hex_value': None,
            'unit_value': None,
            'unit_text': None,
            'is_consistent': True,
        }

        # Try to extract hex value in parentheses
        hex_match = re.search(r'\(0x([0-9a-fA-F]+)\)', value_str)
        if hex_match:
            result['hex_value'] = int(hex_match.group(1), 16)
            result['bytes'] = result['hex_value']

        # Try to extract decimal value with units (kB, MB, B)
        # Support both dot and comma as decimal separators (e.g., 247.8125 or 247,8125)
        unit_match = re.search(r'(\d+(?:[.,]\d+)?)\s*(kB|MB|B)\b', value_str, re.IGNORECASE)
        if unit_match:
            value_str_matched = unit_match.group(1).replace(',', '.')  # Normalize comma to dot
            value, unit = float(value_str_matched), unit_match.group(2).upper()
            result['unit_value'] = value
            result['unit_text'] = unit
            multipliers = {'B': 1, 'KB': 1024, 'MB': 1024 * 1024}
            unit_bytes = int(value * multipliers.get(unit, 1))

            # If we have both hex and unit values, check consistency
            if result['hex_value'] is not None:
                if unit_bytes != result['hex_value']:
                    result['is_consistent'] = False
                # Use hex value as the authoritative source
                result['bytes'] = result['hex_value']
            else:
                # No hex value, use unit conversion
                result['bytes'] = unit_bytes

        # If no hex or unit match found, try standalone hex values
        if result['bytes'] == (None if allow_none else 0):
            hex_match = re.search(r'\b0x([0-9a-fA-F]+)\b', value_str)
            if hex_match:
                result['hex_value'] = int(hex_match.group(1), 16)
                result['bytes'] = result['hex_value']

        # Try plain numbers as last resort
        if result['bytes'] == (None if allow_none else 0):
            number_match = re.search(r'^\s*(\d+)\s*$', value_str)
            if number_match:
                result['bytes'] = int(number_match.group(1))

        return result

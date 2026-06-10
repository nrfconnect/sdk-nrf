#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

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
• documentation.requirements.board_mappings:
        board_id -> RST display name.
• documentation.requirements.skip_mappings:
        board_id -> Excluded from validation.
• documentation.requirements.board_id_to_doc_flash_tab:
        board_id -> ``Reference Matter memory layouts`` tab title.
• documentation.requirements.flash_doc_tabs_skip_dtsi_compare:
        list of tab titles to skip for flash vs DTSI comparison.

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

3. Flash partition DTSI validation:
   - Scans ``samples/matter/**/*.overlay`` for ``#include <samples/matter/*_partitions.dtsi>``
     to map each board target to its partition DTSI file under ``dts/samples/matter/``
   - Parses ``Reference Matter memory layouts`` tables in the requirements RST
   - Compares offset/size for each partition node against the documentation


NOTES:
------
• This check runs once per test session, not per sample
• Validates consistency between multiple sources: samples, snippets, and docs
• Handles board name mappings between different naming conventions
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

import yaml
from internal.checker import MatterSampleTestCase

MATTER_PARTITION_DTSI_DIR = Path('dts') / 'samples' / 'matter'
DIAGNOSTIC_LOGS_RAM_SECTION_TITLE = 'Diagnostic logs RAM memory requirements'
OVERLAY_PARTITION_INCLUDE_RE = re.compile(
    r'#include\s+<samples/matter/([^>]+_partitions\.dtsi)>', re.MULTILINE
)
REG_ASSIGN_RE = re.compile(r'reg\s*=\s*<([^>]+)>', re.MULTILINE)
DT_SIZE_K_RE = re.compile(r'DT_SIZE_K\s*\(\s*([0-9]+)\s*\)')


def _strip_rst_roles(text: str) -> str:
    return re.sub(r':\w+:`([^`]+)`', r'\1', text)


def _parse_reg_cells(reg_inner: str) -> tuple[int, int]:
    """Parse DTS reg = < ... >; into (address, size). Supports hex literals and DT_SIZE_K(n)."""

    def repl_k(m):
        return str(int(m.group(1)) * 1024)

    s2 = DT_SIZE_K_RE.sub(repl_k, reg_inner.strip())
    nums = re.findall(r'0x[0-9a-fA-F]+|\b\d+\b', s2)
    if len(nums) >= 2:
        return int(nums[0], 0), int(nums[1], 0)
    raise ValueError(f'Could not parse reg cells: {reg_inner!r}')


def _find_matching_brace(text: str, open_brace_idx: int) -> int:
    depth = 0
    for j in range(open_brace_idx, len(text)):
        if text[j] == '{':
            depth += 1
        elif text[j] == '}':
            depth -= 1
            if depth == 0:
                return j
    return -1


@dataclass(frozen=True)
class _PartitionNodeHead:
    names: list[str]
    open_brace_idx: int
    end: int


def _is_dts_label_char(ch: str) -> bool:
    return ch.isalnum() or ch == '_'


def _parse_dts_label_before(text: str, colon_idx: int) -> tuple[str, int] | None:
    """Return (label, label_start) for the identifier immediately before ``colon_idx``."""
    label_end = colon_idx
    label_start = colon_idx - 1
    while label_start >= 0 and _is_dts_label_char(text[label_start]):
        label_start -= 1
    label_start += 1
    if label_start >= label_end:
        return None
    return text[label_start:label_end], label_start


def _skip_horizontal_ws(text: str, idx: int) -> int:
    while idx < len(text) and text[idx] in ' \t':
        idx += 1
    return idx


def _skip_horizontal_ws_backward(text: str, idx: int) -> int:
    while idx >= 0 and text[idx] in ' \t':
        idx -= 1
    return idx


def _find_next_partition_node_head(text: str, pos: int) -> _PartitionNodeHead | None:
    """
    Find the next ``label: partition@addr {`` or ``label_a: label_b: partition@addr {`` header.
    Uses string scanning instead of regex to avoid ReDoS (Sonar S5852).
    """
    marker = 'partition@'
    search_from = pos
    while True:
        idx = text.find(marker, search_from)
        if idx < 0:
            return None

        hex_start = idx + len(marker)
        hex_end = hex_start
        while hex_end < len(text) and text[hex_end] in '0123456789abcdefABCDEF':
            hex_end += 1
        if hex_end == hex_start:
            search_from = idx + 1
            continue

        open_brace_idx = _skip_horizontal_ws(text, hex_end)
        if open_brace_idx >= len(text) or text[open_brace_idx] != '{':
            search_from = idx + 1
            continue

        colon_idx = _skip_horizontal_ws_backward(text, idx - 1)
        if colon_idx < 0 or text[colon_idx] != ':':
            search_from = idx + 1
            continue

        trailing_label = _parse_dts_label_before(text, colon_idx)
        if trailing_label is None:
            search_from = idx + 1
            continue
        trailing_name, trailing_start = trailing_label

        prev_idx = _skip_horizontal_ws_backward(text, trailing_start - 1)
        if prev_idx >= 0 and text[prev_idx] == ':':
            leading_label = _parse_dts_label_before(text, prev_idx)
            if leading_label is None:
                search_from = idx + 1
                continue
            leading_name = leading_label
            names = [leading_name, trailing_name]
        else:
            names = [trailing_name]

        return _PartitionNodeHead(
            names=names,
            open_brace_idx=open_brace_idx,
            end=open_brace_idx + 1,
        )


def parse_partition_regs_from_dtsi(dtsi_text: str) -> dict[str, dict[str, int]]:
    """
    Extract partition node labels and reg = <addr size> from a Matter samples .dtsi file.
    Handles nested partition nodes (e.g. TF-M slot0 sub-partitions).
    """
    out: dict[str, dict[str, int]] = {}
    pos = 0
    while True:
        head = _find_next_partition_node_head(dtsi_text, pos)
        if not head:
            break
        names = head.names
        open_idx = head.open_brace_idx
        close_idx = _find_matching_brace(dtsi_text, open_idx)
        if close_idx < 0:
            break
        block = dtsi_text[open_idx : close_idx + 1]
        rm = REG_ASSIGN_RE.search(block)
        if rm:
            addr, size = _parse_reg_cells(rm.group(1).strip())
            for n in names:
                out[n] = {'offset': addr, 'size': size}
        pos = close_idx + 1
    return out


# noqa: kept adjacent to flash-layout helpers
_FLASH_LAYOUT_HEADER_MARKERS = ('Partition', 'Offset', 'Size', 'Element offset', '====')


def _is_rst_grid_separator_line(stripped: str) -> bool:
    return stripped.startswith(('+', '=')) or '+=' in stripped


def _is_flash_layout_header_row(col0: str, col1: str) -> bool:
    return (
        any(marker in col0 for marker in _FLASH_LAYOUT_HEADER_MARKERS)
        or any(marker in col1 for marker in _FLASH_LAYOUT_HEADER_MARKERS)
        or col0.startswith('+')
        or col1.startswith('+')
    )


def _parse_flash_layout_grid_row(line: str) -> list[str] | None:
    """Parse one RST grid line into 6 inner columns, or None if not a data row."""
    if '|' not in line:
        return None
    if _is_rst_grid_separator_line(line.strip()):
        return None
    parts = [c.strip() for c in line.split('|')]
    if len(parts) < 8:
        return None
    inner = parts[1:-1]
    if len(inner) != 6:
        return None
    col0, col1, _, col3, _, _ = inner
    if not col0 and not col3:
        return None
    if _is_flash_layout_header_row(col0, col1):
        return None
    return inner


def _extract_flash_layout_wide_rows(tab_lines: list[str]) -> list[list[str]]:
    """Rows from 6-column RST grid used in Reference Matter memory layouts."""
    rows: list[list[str]] = []
    for line in tab_lines:
        inner = _parse_flash_layout_grid_row(line)
        if inner is not None:
            rows.append(inner)
    return rows


def _partition_names_in_cell(cell: str) -> list[str]:
    cell = _strip_rst_roles(cell)
    return re.findall(r'\(([a-zA-Z0-9_]+)\)', cell)


def _is_valid_element_label(col3: str) -> bool:
    col3s = col3.strip()
    return bool(col3s and col3s != '-' and re.match(r'^[a-zA-Z0-9_]+$', col3s))


def _parse_offset_size_cells(col_offset: str, col_size: str, parse_value) -> dict[str, int] | None:
    if col_offset.strip() in ('-', '') or col_size.strip() in ('-', ''):
        return None
    od = parse_value(col_offset, allow_none=True)
    sd = parse_value(col_size, allow_none=False)
    if od['bytes'] is None or sd['bytes'] is None:
        return None
    return {'offset': od['bytes'], 'size': sd['bytes']}


def _parse_element_partition_from_row(
    col3: str, col4: str, col5: str, parse_value
) -> tuple[str, dict[str, int]] | None:
    """Element columns (network core B0n, TF-M sub-slots, etc.)."""
    if not _is_valid_element_label(col3):
        return None
    values = _parse_offset_size_cells(col4, col5, parse_value)
    if values is None:
        return None
    return col3.strip(), values


def _parse_primary_partition_from_row(
    col0: str, col1: str, col2: str, parse_value
) -> tuple[str, dict[str, int]] | None:
    col0s = col0.strip()
    if not col0s or col0s == '-':
        return None
    if 'Free space' in col0s:
        return None
    names = _partition_names_in_cell(col0s)
    if not names:
        return None
    # e.g. (b0n_partition / provision_partition) — element rows carry the breakdown
    if ' / ' in col0s and '(' in col0s:
        return None
    values = _parse_offset_size_cells(col1, col2, parse_value)
    if values is None:
        return None
    return names[-1], values


def _parse_flash_layout_tab_to_partitions(
    tab_content: str, parse_value
) -> dict[str, dict[str, int]]:
    """
    Build partition_name -> {offset, size} from doc tables (hex in parentheses is canonical).
    parse_value: same contract as CheckDocRequirementsTestCase._parse_value_from_string.
    """
    parts: dict[str, dict[str, int]] = {}
    rows = _extract_flash_layout_wide_rows(tab_content.split('\n'))

    for col0, col1, col2, col3, col4, col5 in rows:
        element = _parse_element_partition_from_row(col3, col4, col5, parse_value)
        if element is not None:
            parts[element[0]] = element[1]
        primary = _parse_primary_partition_from_row(col0, col1, col2, parse_value)
        if primary is not None:
            parts[primary[0]] = primary[1]

    return parts


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
        self._requirements_cache = None  # Cache for parsed diagnostic RAM tables
        self._flash_layout_doc_cache: dict[str, dict[str, dict]] | None = None
        self._board_to_partition_dtsi_cache: dict[str, str] | None = None

    def name(self) -> str:
        return "Hardware requirements documentation check"

    def prepare(self):
        """Prepare the check by reading the requirements file"""
        self.file_path = (
            self.config.config_file.get("documentation").get("requirements").get("path")
        )
        self._requirements_cache = None
        self._flash_layout_doc_cache = None
        self._board_to_partition_dtsi_cache = None
        self._flash_doc_warned_keys: set[str] = set()
        try:
            with open(self.config.nrf_path / self.file_path) as f:
                self.content = f.read()
        except Exception as e:
            self.issue(f"Error reading file: {e}")

    def _requirements_hw_doc_cfg(self) -> dict:
        """``documentation.requirements`` mapping from matter_sample_checker_config.yaml."""
        documentation = self.config.config_file.get('documentation') or {}
        requirements = documentation.get('requirements') or {}
        return requirements if isinstance(requirements, dict) else {}

    def _board_mappings(self) -> dict[str, str]:
        raw = self._requirements_hw_doc_cfg().get('board_mappings')
        if isinstance(raw, dict):
            return {str(k): str(v) for k, v in raw.items()}
        return {}

    def _skip_mappings(self) -> dict[str, str]:
        raw = self._requirements_hw_doc_cfg().get('skip_mappings')
        if isinstance(raw, dict):
            return {str(k): str(v) for k, v in raw.items()}
        return {}

    def _should_skip_board(self, board_id: str) -> bool:
        """Return True when board-specific requirements validation should be skipped."""
        return board_id in self._skip_mappings()

    def _board_id_to_doc_flash_tab(self) -> dict[str, str]:
        raw = self._requirements_hw_doc_cfg().get('board_id_to_doc_flash_tab')
        if isinstance(raw, dict):
            return {str(k): str(v) for k, v in raw.items()}
        return {}

    def _flash_doc_tabs_skip_dtsi_compare(self) -> frozenset[str]:
        raw = self._requirements_hw_doc_cfg().get('flash_doc_tabs_skip_dtsi_compare')
        if isinstance(raw, list):
            return frozenset(str(x) for x in raw)
        return frozenset()

    def _get_display_name(self, board_id: str) -> str:
        """RST / diagnostic tab display name for a board target (overlay stem)."""
        return self._board_mappings().get(board_id, board_id)

    def _get_flash_doc_tab_for_board(self, board_id: str) -> str:
        """RST ``.. tab::`` title under Reference Matter memory layouts for this board."""
        return self._board_id_to_doc_flash_tab().get(board_id, self._get_display_name(board_id))

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

        # Validate flash / fixed-partition DTSI against Reference Matter memory layouts
        self._validate_flash_partitions_doc_vs_dtsi()

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
        has_diag_section = DIAGNOSTIC_LOGS_RAM_SECTION_TITLE in self.content
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
            if self._should_skip_board(board_name):
                if self.config.verbose:
                    self.debug(
                        f"+ Skipping {overlay_file.name} for board {board_name} (skip_mappings)"
                    )
                continue
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

    def _find_flash_layout_section_bounds(self, lines: list[str]) -> tuple[int, int]:
        start = -1
        for i, line in enumerate(lines):
            if 'Reference Matter memory layouts' in line:
                start = i
                break
        if start < 0:
            return -1, -1
        for j in range(start + 1, len(lines)):
            if DIAGNOSTIC_LOGS_RAM_SECTION_TITLE in lines[j]:
                return start, j
        return start, len(lines)

    def _get_parsed_flash_layout_doc_tables(
        self, requirements_path: str
    ) -> dict[str, dict[str, dict]]:
        if self._flash_layout_doc_cache is not None:
            return self._flash_layout_doc_cache
        out: dict[str, dict[str, dict]] = {}
        try:
            lines = Path(requirements_path).read_text(encoding='utf-8').split('\n')
        except OSError as e:
            self.warning(f"Could not read requirements for flash layout: {e}")
            self._flash_layout_doc_cache = out
            return out
        start, end = self._find_flash_layout_section_bounds(lines)
        if start < 0 or end <= start:
            self._flash_layout_doc_cache = out
            return out
        chunk = '\n'.join(lines[start:end])
        tab_pattern = r'.. tab:: (.+?)\n(.*?)(?=.. tab:|$)'
        skip_flash_tabs = self._flash_doc_tabs_skip_dtsi_compare()
        for title, tab_content in re.findall(tab_pattern, chunk, re.DOTALL):
            title = title.strip()
            if title in skip_flash_tabs:
                continue
            parsed = _parse_flash_layout_tab_to_partitions(
                tab_content, self._parse_value_from_string
            )
            if parsed:
                out[title] = parsed
        self._flash_layout_doc_cache = out
        return out

    def _build_board_to_partition_dtsi_map(self) -> dict[str, str]:
        """
        Map board target (overlay stem) -> Matter partition DTSI filename by scanning
        ``samples/matter/**.overlay`` for ``#include <samples/matter/*_partitions.dtsi>``.
        """
        if self._board_to_partition_dtsi_cache is not None:
            return self._board_to_partition_dtsi_cache
        matter_root = self.config.nrf_path / 'samples' / 'matter'
        board_to_dtsi: dict[str, str] = {}
        if not matter_root.is_dir():
            self._board_to_partition_dtsi_cache = board_to_dtsi
            return board_to_dtsi
        for overlay_path in matter_root.rglob('*.overlay'):
            try:
                text = overlay_path.read_text(encoding='utf-8', errors='replace')
            except OSError:
                continue
            incs = OVERLAY_PARTITION_INCLUDE_RE.findall(text)
            if not incs:
                continue
            board_id = overlay_path.name.replace('.overlay', '')
            dtsi_name = incs[0]
            if board_id in board_to_dtsi and board_to_dtsi[board_id] != dtsi_name:
                self.warning(
                    f"Matter partition DTSI include conflict for board {board_id!r}: "
                    f"{board_to_dtsi[board_id]!r} vs {dtsi_name!r} "
                    f"(from {overlay_path.relative_to(self.config.nrf_path)}) — using first"
                )
            else:
                board_to_dtsi[board_id] = dtsi_name
        self._board_to_partition_dtsi_cache = board_to_dtsi
        return board_to_dtsi

    def _log_board_partition_dtsi_map(self, board_to_dtsi: dict[str, str]) -> None:
        if not self.config.verbose:
            return
        self.debug('Board -> partition .dtsi map (from Matter sample overlays):')
        for bid in sorted(board_to_dtsi):
            self.debug(f'  {bid} -> {board_to_dtsi[bid]}')
        rev: dict[str, list[str]] = {}
        for bid, dn in board_to_dtsi.items():
            rev.setdefault(dn, []).append(bid)
        self.debug('Partition .dtsi -> boards:')
        for dn in sorted(rev):
            self.debug(f'  {dn}: {", ".join(sorted(rev[dn]))}')

    def _warn_missing_flash_tab_once(self, board_id: str, tab_title: str, dtsi_name: str) -> None:
        wkey = f'missing-flash-tab:{tab_title}:{dtsi_name}'
        if wkey in self._flash_doc_warned_keys:
            return
        self._flash_doc_warned_keys.add(wkey)
        self.warning(
            f'Flash layout documentation: no tab {tab_title!r} for board {board_id!r} '
            f'(partition DTSI {dtsi_name})'
        )

    def _load_dtsi_partitions(
        self, dtsi_dir: Path, dtsi_name: str, board_id: str
    ) -> dict[str, dict[str, int]] | None:
        dtsi_path = dtsi_dir / dtsi_name
        if not dtsi_path.is_file():
            self.issue(f'Missing partition DTSI file {dtsi_path} (board {board_id})')
            return None
        try:
            dtsi_text = dtsi_path.read_text(encoding='utf-8', errors='replace')
        except OSError as e:
            self.issue(f'Error reading {dtsi_path}: {e}')
            return None
        return parse_partition_regs_from_dtsi(dtsi_text)

    def _validate_one_board_flash_partitions(
        self,
        board_id: str,
        dtsi_name: str,
        flash_tabs: dict[str, dict[str, dict]],
        dtsi_dir: Path,
        skip_flash_tabs: frozenset[str],
        seen_pairs: set[tuple[str, str]],
    ) -> None:
        if self._should_skip_board(board_id):
            if self.config.verbose:
                self.debug(
                    f'Skipping flash partition validation for board {board_id!r} (skip_mappings)'
                )
            return
        tab_title = self._get_flash_doc_tab_for_board(board_id)
        if tab_title in skip_flash_tabs:
            return
        pair = (tab_title, dtsi_name)
        if pair in seen_pairs:
            return
        seen_pairs.add(pair)

        doc_parts = flash_tabs.get(tab_title)
        if not doc_parts:
            self._warn_missing_flash_tab_once(board_id, tab_title, dtsi_name)
            return

        dtsi_parts = self._load_dtsi_partitions(dtsi_dir, dtsi_name, board_id)
        if dtsi_parts is None:
            return
        self._compare_dtsi_partitions_to_doc(board_id, dtsi_name, tab_title, dtsi_parts, doc_parts)

    def _validate_flash_partitions_doc_vs_dtsi(self) -> None:
        """Compare Reference Matter memory layouts tables with
        ``dts/samples/matter/*_partitions.dtsi``.
        """
        requirements_path = str(self.config.nrf_path / self.file_path)
        flash_tabs = self._get_parsed_flash_layout_doc_tables(requirements_path)
        if not flash_tabs:
            self.warning(
                'No parsed flash partition tables (section "Reference Matter memory layouts"'
                + ' missing?)'
            )
            return

        board_to_dtsi = self._build_board_to_partition_dtsi_map()
        dtsi_dir = self.config.nrf_path / MATTER_PARTITION_DTSI_DIR
        self._log_board_partition_dtsi_map(board_to_dtsi)

        seen_pairs: set[tuple[str, str]] = set()
        skip_flash_tabs = self._flash_doc_tabs_skip_dtsi_compare()
        for board_id in sorted(board_to_dtsi):
            self._validate_one_board_flash_partitions(
                board_id,
                board_to_dtsi[board_id],
                flash_tabs,
                dtsi_dir,
                skip_flash_tabs,
                seen_pairs,
            )

    def _compare_dtsi_partitions_to_doc(
        self,
        board_id: str,
        dtsi_name: str,
        tab_title: str,
        dtsi_parts: dict[str, dict[str, int]],
        doc_parts: dict[str, dict[str, int]],
    ) -> None:
        for pname, dv in dtsi_parts.items():
            docv = doc_parts.get(pname)
            if not docv:
                if self.config.verbose:
                    self.debug(
                        f'{board_id} / {dtsi_name}: DTSI partition {pname!r} not listed in doc tab '
                        f'{tab_title!r}'
                    )
                continue
            if docv['offset'] != dv['offset'] or docv['size'] != dv['size']:
                self.issue(
                    f'Flash partition mismatch board={board_id!r} dtsi={dtsi_name!r} '
                    f'doc_tab={tab_title!r} '
                    f'partition={pname!r}: '
                    f'doc offset=0x{docv["offset"]:X} size=0x{docv["size"]:X} vs '
                    f'DTSI offset=0x{dv["offset"]:X} size=0x{dv["size"]:X}'
                )

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
            if DIAGNOSTIC_LOGS_RAM_SECTION_TITLE in line:
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
        req_board_name = self._get_display_name(board_name)

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

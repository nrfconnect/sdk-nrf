#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from pathlib import Path

import yaml


def extract_boards_from_sample_yaml(sample_yaml_path: Path, warnings: list[str]) -> set:
    """Extract all supported board names from a sample.yaml file."""
    if not sample_yaml_path.exists():
        return set()

    try:
        with open(sample_yaml_path, encoding='utf-8') as f:
            sample_data = yaml.safe_load(f)

        boards = set()
        tests = sample_data.get('tests', {})

        for _, test_config in tests.items():
            # Get boards from platform_allow
            platform_allow = test_config.get('platform_allow', [])
            if isinstance(platform_allow, str):
                boards.add(platform_allow)
            elif isinstance(platform_allow, list):
                boards.update(platform_allow)

            # Get boards from integration_platforms
            integration_platforms = test_config.get('integration_platforms', [])
            if isinstance(integration_platforms, list):
                boards.update(integration_platforms)

        # Extract board base names and variants for matching .conf/.overlay files
        board_bases = set()
        for board in boards:
            # Convert board names like 'nrf52840dk/nrf52840' to 'nrf52840dk_nrf52840'
            # and 'nrf5340dk/nrf5340/cpuapp' to 'nrf5340dk_nrf5340_cpuapp'
            board_base = board.replace('/', '_')
            board_bases.add(board_base)

            # Also add base board names for broader matching
            # e.g., from 'nrf5340dk/nrf5340/cpuapp' extract 'nrf5340dk'
            parts = board.split('/')
            if parts:
                board_bases.add(parts[0])

        return board_bases

    except Exception as e:
        warnings.append(f"Could not parse sample.yaml at {sample_yaml_path}: {e}")
        return set()


def is_board_specific_file(file_path: Path, unsupported_boards: set) -> bool:
    """Check if a configuration file is specific to an unsupported board."""
    file_name = file_path.name
    file_path_str = str(file_path)

    # Check if filename contains any unsupported board name
    for board in unsupported_boards:
        if board in file_name:
            return True

    # Additional patterns for board-specific files that might not match exactly
    unsupported_patterns = set()

    for board in unsupported_boards:
        # Add the exact board name
        unsupported_patterns.add(board)

        # Handle different board naming patterns
        if '_' in board:
            # For nrf7002dk_nrf5340_cpuapp, also check nrf7002dk and nrf70ek
            parts = board.split('_')
            if 'nrf7002dk' in parts:
                unsupported_patterns.add('nrf7002dk')
                unsupported_patterns.add('nrf70ek')  # WiFi shield variant

            # For nrf54l15dk_nrf54l15_cpuapp_ns, also check base patterns
            base_board = parts[0]  # e.g., nrf54l15dk
            unsupported_patterns.add(base_board)

            # Handle _internal and _ns variants
            if board.endswith('_ns'):
                base_without_ns = board[:-3]  # Remove _ns
                unsupported_patterns.add(base_without_ns + '_internal')

        # For nrf21540dk and other shield patterns
        if 'nrf21540' in board:
            unsupported_patterns.add('nrf21540dk')
            unsupported_patterns.add('nrf21540ek')

    # Check if file matches any of the patterns
    return any(pattern in file_name or pattern in file_path_str for pattern in unsupported_patterns)

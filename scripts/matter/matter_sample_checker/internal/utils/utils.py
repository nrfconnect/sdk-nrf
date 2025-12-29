#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import subprocess
import sys
from fnmatch import fnmatch
from pathlib import Path
from typing import Any

import yaml

COLORS = {
    "red": "\033[91m",
    "green": "\033[92m",
    "yellow": "\033[93m",
    "blue": "\033[94m",
    "magenta": "\033[95m",
    "cyan": "\033[96m",
    "white": "\033[97m",
}


def colorize(text: str, color_code: str) -> str:
    """Add ANSI color codes to text if colors are enabled."""
    colors_enabled = sys.stdout.isatty()
    if not colors_enabled:
        return text
    RESET = '\033[0m'
    return f"{color_code}{text}{RESET}"


def load_config(config_path: str = None) -> dict[Any, Any]:
    """Load configuration from YAML file."""
    if config_path is None:
        # Try to find config file in the same directory as this script
        script_dir = Path(__file__).parent
        config_path = script_dir.parent / 'matter_sample_checker_config.yaml'

    config_path = Path(config_path)

    if not config_path.exists():
        print(f"Error: Configuration file not found: {config_path}")
        sys.exit(1)

    try:
        with open(config_path, encoding='utf-8') as f:
            return yaml.safe_load(f)
    except Exception as e:
        print(f"Error loading configuration file {config_path}: {e}")
        sys.exit(1)


def find_config_files(directory: Path, config: dict[str, Any]) -> list[Path]:
    """
    Find all configuration files (based on configured extensions) in the given directory
    recursively, excluding build directories and sample-specific files.
    """
    config_files = []

    # Get exclusion patterns from configuration
    exclusions = config.get('exclusions')
    exclude_dirs = set(exclusions.get('exclude_dirs'))
    exclude_files = set(exclusions.get('exclude_files'))
    config_extensions = exclusions.get('exclude_file_extensions')

    # Find all configured file types
    for extension in config_extensions:
        for config_file in directory.rglob(extension):
            # Check if any path part matches any exclude pattern (supporting wildcards)
            excluded = False
            for part in config_file.parts:
                if any(fnmatch(part, exclude_pattern) for exclude_pattern in exclude_dirs):
                    excluded = True
                    break

            if not excluded and config_file.name not in exclude_files:
                config_files.append(config_file)

    return sorted(config_files)


def find_template_directory(sample_path: Path) -> Path | None:
    """Find the matter template directory."""
    # Use the discover_matter_samples logic to find the matter directory
    current = sample_path
    while current != current.parent:
        if current.name == 'matter' and (current.parent / 'matter').exists():
            matter_dir = current
            break
        if (current / 'samples' / 'matter').exists():
            matter_dir = current / 'samples' / 'matter'
            break
        current = current.parent

    if current == current.parent:
        # Fallback: try some common paths
        possible_paths = [
            Path('/home/arbl/ncs/nrf/samples/matter'),
            sample_path.parent,  # If we're already in a matter samples dir
        ]
        for path in possible_paths:
            if path.exists() and path.is_dir():
                matter_dir = path
                break
        else:
            return None

    template_dir = matter_dir / 'template'
    return template_dir if template_dir.exists() else None


def parse_samples_zap_yaml(yaml_path, nrf_base=None):
    """
    Parse a YAML file containing a list of samples to check.

    Args:
        yaml_path: Path to the YAML file
        nrf_base: Base directory for resolving relative paths in the YAML

    Returns:
        List of sample directory paths
    """
    with open(yaml_path) as f:
        data = yaml.safe_load(f)

    if not isinstance(data, list):
        print("Error: YAML file should contain a list of sample entries")
        sys.exit(1)

    samples = []
    yaml_base_dir = None

    # Look for base_dir entry
    for entry in data:
        if not isinstance(entry, dict):
            continue
        if 'base_dir' in entry:
            yaml_base_dir = entry['base_dir']
            break

    # Extract sample paths from zap_file entries
    for entry in data:
        if not isinstance(entry, dict):
            continue

        if 'zap_file' in entry:
            zap_file = entry['zap_file']
            # Extract sample directory from zap_file path
            # e.g., "samples/matter/light_bulb/src/default_zap/light_bulb.zap" ->
            # "samples/matter/light_bulb"
            # or "applications/matter_bridge/src/default_zap/bridge.zap" ->
            # "applications/matter_bridge"
            parts = Path(zap_file).parts

            # Find the sample/application root (parent of 'src')
            if 'src' in parts:
                src_index = parts.index('src')
                sample_rel_path = Path(*parts[:src_index])

                # Skip if path contains 'common'
                if 'common' in sample_rel_path.parts:
                    continue

                # Resolve the full path
                if nrf_base:
                    sample_path = Path(nrf_base) / sample_rel_path
                elif yaml_base_dir:
                    # Use the base_dir from YAML relative to the YAML file location
                    yaml_dir = Path(yaml_path).parent
                    sample_path = yaml_dir / yaml_base_dir / sample_rel_path
                else:
                    sample_path = Path(sample_rel_path)

                sample_path = sample_path.resolve()

                # Add to list if not already present and if it exists
                if sample_path not in samples:
                    if sample_path.exists() and sample_path.is_dir():
                        samples.append(sample_path)
                    else:
                        print(
                            f"Warning: Sample path does not exist or is not a directory: \
                            {sample_path}"
                        )

    return samples


def discover_matter_samples(sample_path: Path) -> list[str]:
    """Discover all Matter samples in the NCS tree."""
    # Try to find the matter samples directory from the current sample path
    matter_dir = None

    # Walk up from current sample to find ncs/nrf/samples/matter
    current = sample_path
    while current != current.parent:  # Stop at filesystem root
        if current.name == 'matter' and (current.parent / 'matter').exists():
            matter_dir = current
            break
        if (current / 'samples' / 'matter').exists():
            matter_dir = current / 'samples' / 'matter'
            break
        current = current.parent

    if not matter_dir:
        # Fallback: try some common paths
        possible_paths = [
            Path('/home/arbl/ncs/nrf/samples/matter'),
            sample_path.parent,  # If we're already in a matter samples dir
        ]
        for path in possible_paths:
            if path.exists() and path.is_dir():
                matter_dir = path
                break

    # Discover all sample directories (exclude 'common' as it's not a sample)
    samples = []
    for item in matter_dir.iterdir():
        if (
            item.is_dir()
            and item.name != 'common'
            and not item.name.startswith('.')
            and ((item / 'sample.yaml').exists() or (item / 'CMakeLists.txt').exists())
        ):
            samples.append(item.name)

    return samples


def find_line_comment_issues(content: str) -> list[tuple]:
    """
    Find line comment issues in file content, excluding comments inside strings and block comments.
    """
    issues = []
    lines = content.split('\n')

    in_block_comment = False
    in_string = False
    string_char = None

    for line_num, line in enumerate(lines, 1):
        i = 0
        while i < len(line):
            char = line[i]

            # Handle string literals
            if not in_block_comment:
                if not in_string and char in ['"', "'"]:
                    in_string = True
                    string_char = char
                elif in_string and char == string_char:
                    # Check for escaped quotes
                    if i == 0 or line[i - 1] != '\\':
                        in_string = False
                        string_char = None

            # Skip if we're inside a string
            if in_string:
                i += 1
                continue

            # Handle block comments
            if not in_block_comment and i < len(line) - 1 and line[i : i + 2] == '/*':
                in_block_comment = True
                i += 2
                continue
            elif in_block_comment and i < len(line) - 1 and line[i : i + 2] == '*/':
                in_block_comment = False
                i += 2
                continue

            # Check for line comments only if we're not in a block comment
            if (
                not in_block_comment
                and not in_string
                and i < len(line) - 1
                and line[i : i + 2] == '//'
            ):
                # Found a line comment - record it
                issues.append((line_num, line))
                break  # Move to next line

            i += 1

        # Reset string state at end of line (strings don't continue across lines in C/C++)
        if in_string:
            in_string = False
            string_char = None

    return issues


def has_internal_build_configs(sample_yaml_path: Path) -> bool:
    """Check if the sample has build configurations that generate _internal files."""
    if not sample_yaml_path.exists():
        return False

    try:
        with open(sample_yaml_path, encoding='utf-8') as f:
            sample_data = yaml.safe_load(f)

        tests = sample_data.get('tests', {})

        for test_config in tests.values():
            extra_args = test_config.get('extra_args', [])
            # Check if any test has FILE_SUFFIX=internal
            if any('FILE_SUFFIX=internal' in str(arg) for arg in extra_args):
                return True

        return False

    except Exception as e:
        print(f"Error checking internal build configs at {sample_yaml_path}: {e}")
        return False


def create_diff_report(diff_result: list[str]) -> str:
    # Show diff lines with line numbers and colors (limit to avoid spam)
    change_count = 0
    context_lines = []

    for line in diff_result:
        # Include context lines (@@) to show line numbers
        if line.startswith('+'):
            change_count += 1
            if change_count <= 10:  # Show more changes in verbose mode
                colored_line = colorize(line.rstrip(), COLORS["green"])
                context_lines.append(f"    {colored_line}")
        elif line.startswith('-'):
            change_count += 1
            if change_count <= 10:  # Show more changes in verbose mode
                colored_line = colorize(line.rstrip(), COLORS["red"])
                context_lines.append(f"    {colored_line}")
        elif line.startswith(' '):
            # Context lines (unchanged)
            if change_count <= 10:
                context_lines.append(f"    {line.rstrip()}")

    return "\n".join(context_lines)


def get_latest_matter_sha(nrf_root: Path) -> str:
    """
    Get latest merge commit SHA from Matter repository that represents synchronization with upstream
    """
    matter_root = nrf_root / ".." / "modules" / "lib" / "matter"
    if not matter_root.exists():
        return "Matter repo not found"

    try:
        # First, try to find the latest merge commit (most recent synchronization)
        result = subprocess.run(
            ['git', 'log', '--merges', '--oneline', '-1'],
            cwd=matter_root,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 and result.stdout.strip():
            merge_sha = result.stdout.split()[0]
            # Get the short version of the SHA
            short_result = subprocess.run(
                ['git', 'rev-parse', '--short', merge_sha],
                cwd=matter_root,
                capture_output=True,
                text=True,
            )
            if short_result.returncode == 0 and short_result.stdout.strip():
                return short_result.stdout.strip()
            return merge_sha[:8]  # Fallback to first 8 characters

        # Fallback 1: Look for commits with "Merge" in the message
        result = subprocess.run(
            ['git', 'log', '--grep=Merge', '--oneline', '-1'],
            cwd=matter_root,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 and result.stdout.strip():
            merge_sha = result.stdout.split()[0]
            return merge_sha[:8] if len(merge_sha) > 8 else merge_sha

        # Fallback 2: Look for commits with "sync" in the message (case insensitive)
        result = subprocess.run(
            ['git', 'log', '--grep=sync', '-i', '--oneline', '-1'],
            cwd=matter_root,
            capture_output=True,
            text=True,
        )
        if result.returncode == 0 and result.stdout.strip():
            sync_sha = result.stdout.split()[0]
            return sync_sha[:8] if len(sync_sha) > 8 else sync_sha

        # Final fallback: get HEAD if no merge commits found
        result = subprocess.run(
            ['git', 'rev-parse', '--short', 'HEAD'], cwd=matter_root, capture_output=True, text=True
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()

    except Exception as e:
        return f"Error: {str(e)}"
    return "Unknown"


def get_matter_sha_link(nrf_root: Path, sha: str = None) -> str:
    """Get GitHub link for Matter repository commit"""
    if not sha:
        sha = get_latest_matter_sha(nrf_root)

    if not sha or sha == "Unknown" or sha.startswith("Error"):
        return "Unknown"

    matter_root = nrf_root / ".." / "modules" / "lib" / "matter"
    try:
        # Try to get full SHA for better link reliability
        full_sha_result = subprocess.run(
            ['git', 'rev-parse', sha], cwd=matter_root, capture_output=True, text=True
        )
        if full_sha_result.returncode == 0 and full_sha_result.stdout.strip():
            full_sha = full_sha_result.stdout.strip()
            return f"https://github.com/nrfconnect/sdk-connectedhomeip/commit/{full_sha}"
        else:
            return f"https://github.com/nrfconnect/sdk-connectedhomeip/commit/{sha}"
    except Exception:
        return f"https://github.com/nrfconnect/sdk-connectedhomeip/commit/{sha}"

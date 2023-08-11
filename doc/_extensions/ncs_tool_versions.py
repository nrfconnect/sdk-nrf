"""
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension adds a custom role, :ncs-tool-version: that can be used as a
placeholder for the actual tool version used. The extension collects
tool versions from the "tool-versions-{os}" and pip requirement files.

The role takes one argument, which is the tool that is inquired about.
The tool must be in all uppercase, only uses underscores and follows the
following pattern:

For tool-versions-{os} files:
    {TOOL_NAME}_VERSION_{OS}
where OS is one of WIN10, LINUX or DARWIN

For pip requirement files:
    {TOOL_NAME}_VERSION

Examples of use:
- :ncs-tool-version:`CMAKE_VERSION_LINUX`
- :ncs-tool-version:`SPHINX_VERSION`
- :ncs-tool-version:`SPHINX_NCS_THEME_VERSION`

"""

from docutils import nodes
from sphinx.application import Sphinx
from typing import List, Dict, Callable
from sphinx.util import logging
import yaml
import re

__version__ = "0.1.0"

logger = logging.getLogger(__name__)


def remove_prefix(s: str, prefix: str) -> str:
    """Remove a prefix from a string, if found."""
    return s[len(prefix) :] if s.startswith(prefix) else s


def parse_pip_requirements(lines: List[str]) -> Dict[str, str]:
    """Create a mapping from a pip requirements file.
    The listed dependencies are mapped to their required versions.
    Exact versions will only have their version displayed, and dependencies
    without a required version are mapped to an empty string.
    Otherwise the pip syntax is used for displaying versions.

    Args:
        lines: A list of pip requirements

    Returns:
        A mapping from each dependency to their required version, if any.
    """

    versions = {}
    for line in lines:
        line = line.strip()
        if line.startswith("#"):
            continue

        # Conditionals (;) are ignored.
        tool, version = re.match(r"([^~><=!;#\s]*)([^;#]*)", line).groups()
        tool = tool.upper().replace("-", "_")
        version = remove_prefix(version.strip(), "==")
        versions[f"{tool}_VERSION"] = version
    return versions


def tool_version_replace(app: Sphinx) -> Callable:
    """Create a version mapping and a role function to replace versions given the
    content of the mapping.
    """

    app.config.init_values()

    versions = {}
    for path in app.config.ncs_tool_versions_host_deps:
        if not path.exists():
            logger.warning(f"Tool version file '{path.as_posix()}' does not exist")
            continue

        os = remove_prefix(path.stem, "tools-versions-").upper()
        if path.suffix in [".yml", ".yaml"]:
            with open(path) as version_file:
                yaml_object = yaml.safe_load(version_file)
            for tool in yaml_object:
                tool_upper = tool.replace("-", "_").upper()
                entry = f"{tool_upper}_VERSION_{os}"
                versions[entry] = yaml_object[tool]["version"]

        elif path.suffix == ".txt":
            for line in path.read_text().strip().split("\n"):
                tool, version = line.split("=")
                tool = tool.replace("-", "_").upper()
                entry = f"{tool}_VERSION_{os}"
                versions[entry] = version

    for path in app.config.ncs_tool_versions_python_deps:
        requirement_lines = path.read_text().strip().split("\n")
        pip_versions = parse_pip_requirements(requirement_lines)
        versions.update(pip_versions)

    def tool_version_role(name, rawtext, text, lineno, inliner, options={}, content=[]):
        if text in versions:
            node = nodes.Text(versions[text])
        else:
            logger.error(f"Could not find tool version for '{text}'")
            node = nodes.Text("")

        return [node], []

    return tool_version_role


def setup(app: Sphinx):
    app.add_config_value("ncs_tool_versions_host_deps", None, "env")
    app.add_config_value("ncs_tool_versions_python_deps", None, "env")
    app.add_role("ncs-tool-version", tool_version_replace(app))

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

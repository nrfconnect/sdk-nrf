"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


External software table extension
#################################

This extension adds a new directive to allow generation of software table
versions based on git describe output..

Usage
-----

- ``ext-sw-table::`` - Renders a table of dependency versions.
- ``:ext-sw-version:`project``` - Displays the version of provided project.

"""

import subprocess
from typing import Any, NamedTuple, override

from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import (
    SphinxDirective,
    SphinxRole,
)
from west.manifest import Manifest

GITHUB_REV_FORMAT = "{url}/commit/{rev}"

__version__ = "0.0.1"


class RepositoryDescriptor(NamedTuple):
    name: str
    remote: str


class RepositoryMetadata(NamedTuple):
    version: str
    release_url: str


REMOTES = (
    RepositoryDescriptor("zephyr", "https://github.com/zephyrproject-rtos/zephyr"),
    RepositoryDescriptor(
        "trusted-firmware-m", "https://github.com/TrustedFirmware-M/trusted-firmware-m"
    ),
    RepositoryDescriptor("mbedtls", "https://github.com/Mbed-TLS/mbedtls"),
)


class ExtSwVersion(SphinxRole):
    @override
    def run(self) -> tuple[list[nodes.Node], list[nodes.system_message]]:
        node = nodes.Text(self.env.external_versions_map[self.text].version)
        return [node], []


class ExtSwTable(SphinxDirective):
    @override
    def run(self) -> list[nodes.Node]:
        table = nodes.table()
        tgroup = nodes.tgroup(cols=2)
        table += tgroup

        tgroup += nodes.colspec(colwidth=1)
        tgroup += nodes.colspec(colwidth=1)

        thead = nodes.thead()
        tgroup += thead
        thead += self._render_header()

        tbody = nodes.tbody()
        tgroup += tbody
        for proj, v in self.env.external_versions_map.items():
            tbody += self._render_row(proj, v)

        return [table]

    def _render_header(self) -> nodes.Node:
        row = nodes.row()

        proj_entry = nodes.entry()
        proj_entry += nodes.paragraph(text="Project")

        version_entry = nodes.entry()
        version_entry += nodes.paragraph(text="Version")

        row += proj_entry
        row += version_entry
        return row

    def _render_row(self, proj_name: str, metadata: RepositoryMetadata) -> nodes.Node:
        row = nodes.row()

        proj_entry = nodes.entry()
        proj_entry += nodes.paragraph(text=proj_name)

        version_entry = nodes.entry()
        ref_paragraph = nodes.paragraph()
        ref_paragraph += nodes.reference(
            metadata.version, metadata.version, internal=False, refuri=metadata.release_url
        )
        version_entry += ref_paragraph

        row += proj_entry
        row += version_entry
        return row


def git_fetch_tags(path: str, remote: str) -> None:
    subprocess.run(
        ["git", "-C", path, "fetch", "--tags", remote],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def git_describe(path: str) -> str:
    return subprocess.check_output(["git", "-C", path, "describe", "--exclude", "ncs-*"]).decode()


def git_get_sha(path: str) -> str:
    return subprocess.check_output(["git", "-C", path, "log", "-n1", "--format=format:%h"]).decode()


def get_versions(app: Sphinx) -> None:
    if hasattr(app.env, "external_versions_map"):
        return

    versions_map: dict[str, RepositoryMetadata] = {}

    manifest = Manifest.from_topdir()
    for name, remote in REMOTES:
        proj = manifest.get_projects([name])[0]

        if not proj.abspath:
            continue

        git_fetch_tags(proj.abspath, remote)

        sha = git_get_sha(proj.abspath)
        metadata = RepositoryMetadata(
            git_describe(proj.abspath), GITHUB_REV_FORMAT.format(url=proj.url, rev=sha)
        )

        versions_map[name] = metadata

    app.env.external_versions_map = versions_map


def setup(app: Sphinx) -> dict[str, Any]:
    app.connect("builder-inited", get_versions)

    app.add_role("ext-sw-version", ExtSwVersion())
    app.add_directive("ext-sw-table", ExtSwTable)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

A sphinx extension for referencing files inside of NCS workspace.

Usage:
    Reference a file directly by an absolute path from sdk-nrf root.
    :ncs-file:`/doc/versions.json`

    Reference a file relative to current file in sdk-nrf part of workspace.
    :ncs-file:`../../samples/Kconfig`

    Reference a file in one of the projects inside the workspace by using
    project name from west.yaml specification. (Only absolute paths allowed
    for references outside of sdk-nrf)
    :ncs-file:`zephyr:/.checkpatch.conf`

    Reference a file with an alias:
    :ncs-file:`sample-alias <zephyr:.checkpatch.conf>`
    :ncs-file:`sample-alias <../../samples/Kconfig>`

    The alias can be quoted if it contgains "<" or ">" characters.
    :ncs-file:`"sample-alias with < character" <zephyr:.checkpatch.conf>`

    Quotes and "<>" characters can be escaped by repeating them.
"""

import os
import re
from pathlib import Path
from typing import Any, override

import utils
from docutils import nodes
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxRole
from west.manifest import Project

__version__ = "0.0.1"

NRF_BASE = Path(__file__).absolute().parents[2]


class NcsFile(SphinxRole):
    _default_repo = "sdk-nrf"
    _link_re = re.compile(
        r"^(?P<alias>(?:(?:\"(?:[^\"]|\"\")*\")|(?:'(?:[^']|'')*')|(?:[^\"'].*))\s*(?=<))?"
        # match either quoted string using '' or "" as escapes or unquoted string
        r"(?P<path>(?:<(?:[^<>]|<<|>>)*>)|(?:[^<]+))$"
        # match eithere a string in <> using << and >> as escapes or any string without <
    )

    @staticmethod
    def format_alias(raw: str) -> str:
        res = raw.strip()
        if res.startswith('"') or res.startswith("'"):
            res = res[1:-1]
        return res.replace('""', '"').replace("''", "'")

    @staticmethod
    def format_path(raw: str) -> str:
        res = raw.strip()
        if res.startswith("<"):
            res = res[1:-1]
        return res.replace("<<", "<").replace(">>", ">")

    def get_page_prefix(self, path: str) -> str | None:
        found_prefix = None
        for pattern, prefix in self.env.app.config.gh_link_prefixes.items():
            if re.match(pattern, path):
                found_prefix = prefix
                break
        return found_prefix

    def gh_link_get_url(self, path: str, mode: str = "blob") -> str:
        page_prefix = self.get_page_prefix(path)
        if page_prefix is None:
            raise FileNotFoundError

        if not (NRF_BASE / page_prefix / path).exists():
            raise FileNotFoundError

        return "/".join(
            [
                self.env.app.config.gh_link_base_url,
                mode,
                self.env.app.config.gh_link_version,
                page_prefix,
                path,
            ]
        )

    def error(self, info: str) -> tuple[list[nodes.Node], list[nodes.system_message]]:
        msg = self.inliner.reporter.error(info, line=self.lineno)
        prb = self.inliner.problematic(self.rawtext, self.rawtext, msg)
        return [prb], [msg]

    def get_url_non_nrf(self, project: Project, path: str | os.PathLike) -> str:
        path = Path(path)
        revision = project.revision
        url = project.url
        path = path.relative_to(path.anchor)
        if not os.path.exists(os.path.join(project.abspath, path)):
            raise FileNotFoundError
        return os.path.join(url, "blob", revision, path)

    def parse_path(self, path: str) -> tuple[str, str]:
        colon = path.find(":")

        if colon != -1:
            selected_proj = path[:colon].strip()
            path = path[colon + 1 :]
            if selected_proj != self._default_repo:
                proj = utils.MANIFEST.get_projects([selected_proj])[0]
                return os.path.basename(path), self.get_url_non_nrf(proj, path)

        if os.path.isabs(path):
            tmp = Path(path)
            path = tmp.relative_to(tmp.anchor).as_posix()
            if not (NRF_BASE / path).exists():
                raise FileNotFoundError
            return os.path.basename(path), os.path.join(
                self.config.gh_link_base_url, "blob", self.config.gh_link_version, path
            )

        path = (
            self.env.doc2path(self.env.docname).relative_to(self.env.app.srcdir).parent / path
        ).as_posix()
        return os.path.basename(path), self.gh_link_get_url(path)

    @override
    def run(self) -> tuple[list[nodes.Node], list[nodes.system_message]]:
        match = self._link_re.match(self.text)
        if not match:
            return self.error("Failed to parse ncs-file content")

        if not (path := match.group("path")):
            return self.error("Failed to parse path out of ncs-file content")

        try:
            alias, url = self.parse_path(self.format_path(path))
        except FileNotFoundError:
            return self.error(f"Failed to locate file {path} in directive {self.text}")

        if match.group("alias"):
            alias = self.format_alias(match.group("alias"))
        return [nodes.reference(self.text, alias, refuri=url)], []


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_role("ncs-file", NcsFile())
    return {
        "version": __version__,
        "env_version": 1,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

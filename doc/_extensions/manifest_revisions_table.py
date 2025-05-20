"""
Manifest Revisions Table
========================

This extension allows to render a table containing the revisions of the projects
present in a manifest file.

Usage
*****

This extension introduces a new directive: ``manifest-revisions-table``. It can
be used in the code as::

    .. manifest-revisions-table::
        :show-first: project1, project2

where the ``:show-first:`` option can be used to specify which projects should
be shown first in the table.

Options
*******

- ``manifest_revisions_table_manifest``: Path to the manifest file.

Copyright (c) Nordic Semiconductor ASA 2022
SPDX-License-Identifier: Apache-2.0
"""

import re
from typing import Any, Dict, List

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.errors import ExtensionError
from sphinx.util.docutils import SphinxDirective
from west.manifest import Manifest


__version__ = "0.1.0"


class ManifestRevisionsTable(SphinxDirective):
    """Manifest revisions table."""

    option_spec = {
        "show-first": directives.unchanged,
    }

    @staticmethod
    def rev_url(base_url: str, rev: str) -> str:
        """Return URL for a revision.

        Notes:
            Revision format is assumed to be a git hash or a tag. URL is
            formatted assuming a GitHub base URL.

        Args:
            base_url: Base URL of the repository.
            rev: Revision.

        Returns:
            URL for the revision.
        """

        # remove .git from base_url, if present
        base_url = base_url.split(".git")[0]

        if "github.com" in base_url:
            commit_fmt = base_url + "/commit/{rev}"
            tag_fmt = base_url + "/releases/tag/{rev}"
        elif "projecttools.nordicsemi.no" in base_url:
            m = re.match(r"^(.*)/scm/(.*)/(.*)$", base_url)
            if not m:
                raise ExtensionError(f"Invalid base URL: {base_url}")

            base_url = f"{m.group(1)}/projects/{m.group(2)}/repos/{m.group(3)}"
            commit_fmt = base_url + "/commits/{rev}"
            tag_fmt = commit_fmt
        else:
            raise ExtensionError(f"Unsupported SCM: {base_url}")

        if re.match(r"^[0-9a-f]{40}$", rev):
            return commit_fmt.format(rev=rev)

        return tag_fmt.format(rev=rev)

    def run(self) -> List[nodes.Element]:
        # parse show-first option
        show_first_raw = self.options.get("show-first", None)
        show_first = (
            [entry.strip() for entry in show_first_raw.split(",")]
            if show_first_raw
            else []
        )

        # sort manifest projects accounting for show-first
        manifest = Manifest.from_file(self.env.config.manifest_revisions_table_manifest)
        projects = [None] * len(show_first)
        for project in manifest.projects:
            if project.name == "manifest":
                continue

            if project.name in show_first:
                projects[show_first.index(project.name)] = project
            else:
                projects.append(project)

        if not all(projects):
            raise ExtensionError(f"Invalid show-first option: {show_first_raw}")

        # build table
        table = nodes.table()

        tgroup = nodes.tgroup(cols=2)
        tgroup += nodes.colspec(colwidth=1)
        tgroup += nodes.colspec(colwidth=1)
        table += tgroup

        thead = nodes.thead()
        tgroup += thead

        row = nodes.row()
        thead.append(row)

        entry = nodes.entry()
        entry += nodes.paragraph(text="Project")
        row += entry
        entry = nodes.entry()
        entry += nodes.paragraph(text="Revision")
        row += entry

        rows = []
        for project in projects:
            row = nodes.row()
            rows.append(row)

            entry = nodes.entry()
            entry += nodes.paragraph(text=project.name)
            row += entry
            entry = nodes.entry()
            par = nodes.paragraph()
            par += nodes.reference(
                project.revision,
                project.revision,
                internal=False,
                refuri=ManifestRevisionsTable.rev_url(project.url, project.revision),
            )
            entry += par
            row += entry

        tbody = nodes.tbody()
        tbody.extend(rows)
        tgroup += tbody

        return [table]


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_config_value("manifest_revisions_table_manifest", None, "env")

    directives.register_directive("manifest-revisions-table", ManifestRevisionsTable)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

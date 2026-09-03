"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import subprocess
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from sphinx.util.parsing import nested_parse_to_nodes
from versions import get_versions
from vscode_button.common import render_vscode_button

CONTENTS_DIRECTIVE = """
.. contents::
   :local:
   :depth: 2

"""

OPEN_IN_VSCODE_URL = (
    "vscode://nordic-semiconductor.nrf-connect/openSampleFromSDK"
    "?samplePath={sample}"
    "&sdkVersion={version}"
    "&sdkType=nRF%20Connect%20SDK"
    "&openAddBuildView=true"
)


class NCSSampleDirective(SphinxDirective):
    has_content = True
    required_arguments = 0
    optional_arguments = 0
    option_spec = {
        "title": directives.unchanged,
    }

    def render_contents(self) -> list[nodes.Node]:
        return nested_parse_to_nodes(
            self.state,
            CONTENTS_DIRECTIVE,
            source=self.env.doc2path(self.env.docname).as_posix(),
            allow_section_headings=True,
        )

    def render_header(self) -> nodes.section:
        title = self.options["title"]

        header_section = nodes.section()
        textnodes, messages = self.state.inline_text(title, self.lineno)
        header_section += nodes.title(title, "", *textnodes)
        header_section += messages
        self.state.document.note_implicit_target(header_section, header_section)
        return header_section

    def render_description(self) -> list[nodes.Node]:
        return self.parse_content_to_nodes(allow_section_headings=True)

    def sample_existed(self, sample: str, version: str) -> bool:
        return (
            subprocess.run(
                ["git", "cat-file", "-e", f"{version}:{sample}"],
                stderr=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
            ).returncode
            == 0
        )

    def get_vscode_uri(self, version: str) -> str | None:
        file = self.env.doc2path(self.env.docname).relative_to(self.env.app.srcdir)
        out_dirname = file.parent
        docset = Path(self.config.html_theme_options["docset"])

        if not self.sample_existed(file.as_posix(), f"v{version}"):
            return None

        return OPEN_IN_VSCODE_URL.format(
            version=version,
            sample=(docset / out_dirname).as_posix(),
        )

    def render_vscode_button(self) -> list[nodes.Node]:
        version = get_versions(self.env.app).normalized().patchlevel().latest()
        if not version:
            return []

        uri = self.get_vscode_uri(version)
        if uri:
            return render_vscode_button(
                self.env,
                label="Open in VS Code",
                uri=uri,
                tooltip=(
                    f"Opening the sample may trigger installation of the SDK "
                    f"and toolchain v{version}."
                ),
            )

        return render_vscode_button(
            self.env,
            label="Open in VS Code",
            uri=None,
            tooltip="This sample hasn't been included in an SDK release yet.",
            disabled=True,
        )

    def parent_remainder(self, section: nodes.section) -> None:
        """Parse remaining nodes in file/section directly to be their parent for proper rendering"""
        offset = self.state_machine.line_offset + 1
        new_offset = self.state.nested_parse(
            self.state_machine.input_lines[offset:],
            input_offset=self.state_machine.abs_line_offset() + 1,
            node=section,
            match_titles=True,
        )
        self.state.goto_line(new_offset)

    def run(self) -> list[nodes.Node]:
        section = self.render_header()

        section += self.render_vscode_button()
        section += self.render_contents()
        section += self.render_description()

        self.parent_remainder(section)

        return [section]

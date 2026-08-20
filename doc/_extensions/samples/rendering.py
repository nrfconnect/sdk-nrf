"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import os
import subprocess
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.config import Config
from sphinx.util.docutils import SphinxDirective
from sphinx.util.parsing import nested_parse_to_nodes
from versions import get_versions

VSCODE_IMAGE_NAME = "vscode.svg"
VSCODE_IMAGE_PATH = os.path.join(os.path.dirname(__file__), "static")

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


def add_vscode_image(app: Sphinx, _: Config) -> None:
    app.config.html_static_path.append(VSCODE_IMAGE_PATH)


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

    @staticmethod
    def render_tooltip(text: str) -> nodes.Node:
        return nodes.raw("", f'<span class="vscode-button-tooltip">{text}</span>', format="html")

    def render_vscode_button(self) -> list[nodes.Node]:
        version = get_versions(self.env.app).normalized().patchlevel().latest()
        if not version:
            return []

        svg_path = (
            self.env.app.srcdir.relative_to(self.env.doc2path(self.env.docname), walk_up=True)
            / "nrf/_static/vscode.svg"
        )
        ref_children: list[nodes.Node] = [
            nodes.raw("", f'<img src="{svg_path}"/>', format="html"),
            nodes.Text("Open in VS Code"),
        ]

        uri = self.get_vscode_uri(version)

        if not uri:
            sidebar = nodes.sidebar(classes=["vscode-button", "vscode-button-disabled"])
            sidebar += nodes.paragraph("", "", *ref_children)
            sidebar += self.render_tooltip(
                "This sample hasn't been included in an SDK release yet."
            )
            return [sidebar]

        ref = nodes.reference("", "", *ref_children, refuri=uri)

        ref_p = nodes.paragraph()
        ref_p += ref

        sidebar = nodes.sidebar(classes=["vscode-button"])
        sidebar += ref_p
        sidebar += self.render_tooltip(
            f'Opening the sample may trigger installation of the SDK and toolchain v{version}.'
        )
        return [sidebar]

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

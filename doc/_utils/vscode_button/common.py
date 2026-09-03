"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import re
from pathlib import Path

from docutils import nodes
from sphinx.application import Sphinx
from sphinx.config import Config
from sphinx.environment import BuildEnvironment
from versions import get_versions

STABLE_VERSION_RE = re.compile(r"\d+\.\d+\.\d+(?:-(?!preview\d+|rc\d+|dev\d+).*)?$")

BUTTON_CSS_PATH = Path(__file__).parent / "static"


def add_button_css(app: Sphinx, _: Config) -> None:
    if getattr(app, "_vscode_button_css_added", False):
        return

    app._vscode_button_css_added = True
    app.config.html_static_path.append(BUTTON_CSS_PATH.as_posix())
    app.add_css_file("button.css")


def get_latest_stable_version(app: Sphinx) -> str | None:
    return get_versions(app).normalized().patchlevel().matching(STABLE_VERSION_RE).latest()


def render_tooltip(text: str) -> nodes.Node:
    return nodes.raw("", f'<span class="vscode-button-tooltip">{text}</span>', format="html")


def get_vscode_svg_path(env: BuildEnvironment) -> str:
    depth = len(Path(env.docname).parent.parts)
    prefix = "/".join([".."] * depth) if depth else "."
    return f"{prefix}/_static/vscode.svg"


def render_vscode_button(
    env: BuildEnvironment,
    *,
    label: str,
    uri: str | None,
    tooltip: str,
    disabled: bool = False,
    inline: bool = False,
) -> list[nodes.Node]:
    ref_children: list[nodes.Node] = [
        nodes.raw("", f'<img src="{get_vscode_svg_path(env)}"/>', format="html"),
        nodes.Text(label),
    ]

    classes = ["vscode-button"]
    if inline:
        classes.append("vscode-button-inline")
    if disabled or not uri:
        classes.append("vscode-button-disabled")

    wrapper = nodes.container(classes=classes) if inline else nodes.sidebar(classes=classes)

    if not disabled and uri:
        ref = nodes.reference("", "", *ref_children, refuri=uri)
        ref_p = nodes.paragraph()
        ref_p += ref
        wrapper += ref_p
    else:
        wrapper += nodes.paragraph("", "", *ref_children)

    wrapper += render_tooltip(tooltip)
    return [wrapper]

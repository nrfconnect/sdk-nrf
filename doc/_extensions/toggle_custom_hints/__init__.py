"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Extends the ``toggle`` directive from ``sphinx_togglebutton`` with per-toggle hint text.

Without options, the toggle uses the global ``togglebutton_hint`` and
``togglebutton_hint_hide`` texts (by default, "Click to show" and "Click to hide").

With the ``:custom-hints:`` option, the directive title replaces both texts.
The optional ``:hide-hint:`` option sets the text shown when the toggle is open.

Example of use:
.. toggle:: Installing from Open VSX Registry
   :custom-hints:
   :hide-hint: Hide Open VSX Registry installation

   Content.
"""

from pathlib import Path
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx_togglebutton import Toggle

__version__ = "0.0.1"

RESOURCES_DIR = Path(__file__).parent / "static"


class ToggleContainer(nodes.container):
    pass


class CustomHintToggle(Toggle):
    option_spec = {
        **Toggle.option_spec,
        "custom-hints": directives.flag,
        "hide-hint": directives.unchanged,
    }

    def run(self) -> list[nodes.Node]:
        if "custom-hints" not in self.options:
            return super().run()

        if not self.arguments:
            raise self.error("The :custom-hints: option requires a toggle title.")

        self.assert_has_content()
        classes = ["toggle", "toggle-custom-hints"]
        if "show" in self.options:
            classes.append("toggle-shown")

        node = ToggleContainer(classes=classes)
        node["show_hint"] = self.arguments[0]
        node["hide_hint"] = self.options.get("hide-hint") or self.arguments[0]
        self.state.nested_parse(self.content, self.content_offset, node)
        return [node]


def toggle_container_visit_html(self, node: ToggleContainer) -> None:
    self.body.append(
        self.starttag(
            node,
            "div",
            CLASS="docutils container",
            **{
                "data-toggle-show": node["show_hint"],
                "data-toggle-hide": node["hide_hint"],
            },
        )
    )


def toggle_container_depart_html(self, node: ToggleContainer) -> None:
    self.body.append("</div>\n")


def add_toggle_resources(app: Sphinx) -> None:
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())


def setup(app: Sphinx) -> dict[str, Any]:
    app.setup_extension("sphinx_togglebutton")
    app.add_directive("toggle", CustomHintToggle, override=True)
    app.add_node(
        ToggleContainer,
        html=(toggle_container_visit_html, toggle_container_depart_html),
    )
    app.connect("builder-inited", add_toggle_resources)
    app.add_js_file("toggle_custom_hints.js", priority=501)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

"""
Copyright (c) 2022 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

This extension provides a ``FilterDropdown`` node that generates a dropdown menu
from a list of options, and invokes a javascript filter module that hides all
hideable html elements that does not share a classname with the selected option.

All hideable html elements must include the classname "hideable".

Multiple dropdown nodes can be present one the same page. Only elements with html
classes that include all selected dropdown options will be shown. All dropdown
nodes comes with a "show all" option selected by default.

This extension also provides two directives that use the ``FilterDropdown`` node,
``page-filter`` and ``version-filter``.

The ``page-filter`` directive requires a name and a list of options as its body.
The name is used both as an identifier and to display the default "All x" option.
Every option in the option list should contain the option value contained in one
word, followed by the displayed text for that option.

Example of use:
.. page-filter:: components
    ble_controller  BLE Controller
    nrfcloud        nRFCloud
    tf-m            TF-M
    ble_mesh        BLE MESH

The ``version-filter`` directive provides a pre-populated dropdown-list of all
NRF versions to date. Relevant html elements should include all applicable
versions in its classname on a "vX-X-X" format. The directive also accepts an
optional html tag name as its body. If present, all of the specified tags
that contains one or more versions in its classname will receive the "hideable"
classname and a child paragraph containing a list of corresponding clickable
versiontags.

Example of use:
.. version-filter:: div
"""

from docutils import nodes
from sphinx.util.docutils import SphinxDirective
from sphinx.application import Sphinx
from sphinx.util import logging
from typing import Dict, List, Optional, Tuple
from pathlib import Path
import json

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

RESOURCES_DIR = Path(__file__).parent / "static"
"""Static resources"""

VERSIONS_FILE = Path(__file__).parents[1] / "versions.json"
"""Contains all versions to date"""


class PageFilter(SphinxDirective):

    has_content = True

    def run(self):
        name = self.content[0]
        split_first = lambda s: s.split(maxsplit=1)
        content = list(map(split_first, self.content[1:]))
        return [FilterDropdown(name, content)]


class VersionFilter(PageFilter):
    def run(self):
        container_element = self.content[0] if self.content else None
        create_tuple = lambda v: (v, v.replace("-", "."))
        versions = list(map(create_tuple, reversed(self.env.nrf_versions)))
        return [FilterDropdown("versions", versions, container_element)]


class FilterDropdown(nodes.Element):
    """Generate a dropdown menu for filter selection.

    Args:
        name: Unique identifier, also used in the default "all" option.
        options: List of tuples where the first element is the html value
                 and the second element is the displayed option text
        container_element: html tag to generate version tags in (default None)
    """

    def __init__(
        self, name: str, options: List[Tuple[str, str]], container_element: str = None
    ) -> None:
        super().__init__()
        self.name = name
        self.options = options
        self.container_element = container_element

    def html(self):
        opt_list = [f'<option value="all" selected>All {self.name}</option>']
        for val, text in self.options:
            opt_list.append(f'<option value="{val}">{text}</option>')

        html_str = f'<select name="{self.name}" id="dropdown-select">\n\t'
        html_str += "\n\t".join(opt_list)
        html_str += "\n</select>\n"
        return html_str


def filter_dropdown_visit_html(self, node: nodes.Node) -> None:
    self.body.append(node.html())
    raise nodes.SkipNode


class _FindFilterDropdownVisitor(nodes.NodeVisitor):
    def __init__(self, document):
        super().__init__(document)
        self._found_dropdowns = []

    def unknown_visit(self, node: nodes.Node) -> None:
        if isinstance(node, FilterDropdown):
            self._found_dropdowns.append(node)

    @property
    def found_filter_dropdown(self) -> List[nodes.Node]:
        return self._found_dropdowns


def page_filter_install(
    app: Sphinx,
    pagename: str,
    templatename: str,
    context: Dict,
    doctree: Optional[nodes.Node],
) -> None:
    """Install the javascript filter function."""

    if app.builder.format != "html" or not doctree:
        return

    visitor = _FindFilterDropdownVisitor(doctree)
    doctree.walk(visitor)
    if visitor.found_filter_dropdown:
        app.add_css_file("page_filter.css")
        context["css_files"].append(app.builder.css_files[-1])
        app.add_js_file("page_filter.js", type="module")
        filename = app.builder.script_files[-1]

        body = f"import setupFiltering from './{filename}'; "
        for dropdown in visitor.found_filter_dropdown:
            if dropdown.container_element:
                body += f"setupFiltering('{dropdown.name}', '{dropdown.container_element}'); "
            else:
                body += f"setupFiltering('{dropdown.name}'); "

        app.add_js_file(filename=None, body=body, type="module")
        context["script_files"].append(app.builder.script_files[-1])


def add_filter_resources(app: Sphinx):
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())
    read_versions(app)


def read_versions(app: Sphinx) -> None:
    """Get all NRF versions to date"""

    if hasattr(app.env, "nrf_versions") and app.env.nrf_versions:
        return

    try:
        with open(VERSIONS_FILE) as version_file:
            # Exclude "latest"
            nrf_versions = json.loads(version_file.read())["VERSIONS"][1:]
            # Versions classes are on the format "vX-X-X"
            app.env.nrf_versions = [
                f"v{version.replace('.', '-')}" for version in reversed(nrf_versions)
            ]
    except FileNotFoundError:
        logger.error("Could not load version file")
        app.env.nrf_versions = []


def setup(app: Sphinx):
    app.add_directive("page-filter", PageFilter)
    app.add_directive("version-filter", VersionFilter)

    app.connect("builder-inited", add_filter_resources)
    app.connect("html-page-context", page_filter_install)

    app.add_node(FilterDropdown, html=(filter_dropdown_visit_html, None))

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

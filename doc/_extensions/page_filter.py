"""
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension provides a ``FilterDropdown`` node that generates a dropdown menu
from a list of options, and invokes a javascript filter module that hides all
hideable html elements that does not share a classname with the selected option.

All hideable html elements must include the classname "hideable".

Multiple dropdown nodes can be present one the same page. Only elements with html
classes that include all selected dropdown options will be shown. All dropdown
nodes comes with a "show all" option selected by default, unless set by the
:default: option.

This extension also provides two directives that use the ``FilterDropdown`` node,
``page-filter`` and ``version-filter``.

The ``page-filter`` directive requires a name and a list of options as its body.
The name is used both as an identifier and to display the default "All x" option.
Every option in the option list should contain the option value contained in one
word, followed by the displayed text for that option. If the option value is
prefixed by "!", it will hide the option values instead of show them. An optional
:default: option can be used to change the default selected value from "all".

The directive can also generate a visible HTML div element containing a list of
tags. To do this, both the :tags: and :container: options must be present.
The :tags: option should contain a list of tuples (c, d) where c is the classname
the tag will be generated from, and d is the displayed string in the generated tag.
The :container: option should contain an HTML tag "path", where the top level
element is searched for the specified classnames, and the bottom level element is
the parent element for the taglist div. For example, :container: section/a/span
will search every <section> element for the classnames given in :tags:, create a
tag for each and place them in a div inserted into the <span> element, if such
an element exists within an <a> element within the <section> element in question.

Example of use:
.. page-filter::
  :name: components

  ble_controller  BLE Controller
  nrfcloud        nRFCloud
  tf-m            TF-M
  ble_mesh        BLE MESH

The ``version-filter`` directive provides a pre-populated dropdown-list of all
NRF versions to date. Relevant html elements should include all applicable
versions in its classname on a "vX-X-X" format.
The :name: option is defaulted to "versions".
The :tags: option is prepopulated with the tuple ("versions", ""). This
will generate clickable version tags in addition to any other tags specified,
given that the :container: option is present.

Example of use:
.. version-filter::
  :default: v2-4-0
  :tags: [("wontfix", "Won't fix")]
  :container: dl/dt
"""

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from sphinx.application import Sphinx
from sphinx.util import logging
from typing import Dict, List, Optional, Tuple
from pathlib import Path
import json
import re

__version__ = "0.1.0"

logger = logging.getLogger(__name__)

RESOURCES_DIR = Path(__file__).parent / "static"
"""Static resources"""

VERSIONS_FILE = Path(__file__).parents[1] / "versions.json"
"""Contains all versions to date"""


class PageFilter(SphinxDirective):

    has_content = True
    option_spec = {
        "name": directives.unchanged,
        "default": directives.unchanged,
        "tags": eval,
        "container": directives.unchanged,
    }

    def run(self):
        name = self.options.get("name", "")
        split_first = lambda s: s.split(maxsplit=1)
        content = list(map(split_first, self.content))
        default = self.options.get("default", "all")
        container = self.options.get("container", None)
        tags = self.options.get("tags", [])
        tags = {classname: displayname for classname, displayname in tags}
        return [FilterDropdown(name, content, default, container, tags)]


class VersionFilter(PageFilter):

    has_content = False

    def run(self):
        name = self.options.get("name", "versions")
        default = self.options.get("default", "all")
        container_element = self.options.get("container", None)
        tags = self.options.get("tags", [])
        tags = {classname: displayname for classname, displayname in tags}
        tags["versions"] = ""
        create_tuple = lambda v: (v, v.replace("-", "."))
        versions = list(map(create_tuple, reversed(self.env.nrf_versions)))
        return [FilterDropdown(name, versions, default, container_element, tags)]


class FilterDropdown(nodes.Element):
    """Generate a dropdown menu for filter selection.

    Args:
        name: Unique identifier, also used in the "all" option.
        options: List of tuples where the first element is the html value
                 and the second element is the displayed option text.
        default_value: Value selected by default.
        container_element: html tag to generate filter tags in (default None).
        filter_tags: Tuples of (classname, displayname), where classname is the
                     class to create a tag from, and displayname is the content
                     of the tag.
    """

    def __init__(
        self,
        name: str,
        options: List[Tuple[str, str]],
        default_value: str = "all",
        container_element: str = None,
        filter_tags: Tuple[str, str] = None,
    ) -> None:
        super().__init__()
        self.name = name
        self.options = options
        self.default_value = default_value
        self.container_element = container_element
        self.filter_tags = filter_tags

    def html(self):
        self.options.insert(0, ("all", f"All {self.name}"))
        opt_list = []
        for val, text in self.options:
            if val == self.default_value:
                opt_list.append(f'<option value="{val}" selected>{text}</option>')
            else:
                opt_list.append(f'<option value="{val}">{text}</option>')

        html_str = f'<select name="{self.name}" class="dropdown-select" id =dropdown-select-{self.name}>\n\t'
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
        app.add_js_file("page_filter.mjs", type="module")
        filename = app.builder.script_files[-1]

        page_depth = len(Path(pagename).parents) - 1
        body = f"import setupFiltering from './{page_depth * '../'}{filename}'; "
        for dropdown in visitor.found_filter_dropdown:
            body += f"setupFiltering('{dropdown.name}'"
            if dropdown.container_element and dropdown.filter_tags:
                body += f", '{dropdown.container_element}', {dropdown.filter_tags}"
            body += "); "

        app.add_js_file(filename=None, body=body, type="module")


def add_filter_resources(app: Sphinx):
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())
    read_versions(app)


def read_versions(app: Sphinx) -> None:
    """Get all NRF versions to date"""

    if hasattr(app.env, "nrf_versions") and app.env.nrf_versions:
        return

    try:
        with open(VERSIONS_FILE) as version_file:
            nrf_versions = json.loads(version_file.read())
            nrf_versions = list(
                filter(lambda v: re.match(r"\d\.\d\.\d$", v), nrf_versions)
            )
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
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

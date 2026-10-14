"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

The ``nrfutil-command`` directive.

It renders like ``code-block`` with ``console`` as the default language, and in addition:

* expands substitutions such as ``|release|`` in the block,
* validates every ``nrfutil`` command in the block against a snapshot of the ``--json-args``
  output of the pinned nrfutil versions, unless the ``:no-validate:`` option is set,
* shows placeholders such as ``<serial_number>`` in italics, and accepts them as any value,
* adds a button that opens the nRF Util documentation of the first documented command in the
  block.

The validation and the documentation button need ``nrfutil_command_check``, which the
``NRFUTIL_COMMAND_CHECK`` CMake option sets. The snapshot is generated when the build starts if it
doesn't hold the pinned versions. This needs nrfutil on the PATH. Without it, the build warns.

Validation problems are reported as Sphinx warnings at the file and line of the command.
Commands inside ``Windows``, ``Linux`` and ``macOS`` tabs are validated only if the snapshot comes
from that operating system, because the ``--json-args`` output differs between operating systems.
The ``:os:`` option sets the operating systems of a block instead, for example ``:os: windows``.
"""

from __future__ import annotations

import functools
import html
import re
import shutil
from pathlib import Path
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.directives.code import CodeBlock
from sphinx.transforms import SphinxTransform
from sphinx.util import logging

from nrfutil_command import lexers, specs
from nrfutil_command.generate import SnapshotError, generate
from nrfutil_command.walker import ValidationError, iter_commands, validate_command

logger = logging.getLogger(__name__)

BLOCK_CLASS = "nrfutil-command"
# The snapshot directory selected for the build and its stamp, set when the build starts
ENV_ATTRIBUTE = "nrfutil_command_snapshot"
DOCS_BUTTON_CLASS = "nrfutil-command-docs-button"
DOCS_LABEL = "Open the nRF Util command documentation."
DOCS_TOOLTIP = "Open command docs"
DOCS_ICON = (
    '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke-width="2" '
    'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">'
    '<path d="M3 19a9 9 0 0 1 9 0a9 9 0 0 1 9 0"/><path d="M3 6a9 9 0 0 1 9 0a9 9 0 0 1 9 0"/>'
    '<path d="M3 6v13"/><path d="M12 6v13"/><path d="M21 6v13"/></svg>'
)
STATIC_DIR = Path(__file__).parent / "static"

SUBSTITUTION_RE = re.compile(r"\|([^|\s][^|]*)\|")
# Sphinx resolves these substitutions from the configuration, not from substitution definitions
SPHINX_CONFIG_SUBSTITUTIONS = ("release", "version", "today")


class nrfutil_command_block(nodes.container):
    """The literal block of an nrfutil-command directive and the controls that go with it.

    The directive sets the attributes that the transform needs, and the transform sets the
    attributes that the HTML translator renders. Other builders render it as a container.
    """


class NrfutilCommand(CodeBlock):
    """A code-block for nrfutil commands, validated during the documentation build."""

    required_arguments = 0
    optional_arguments = 1
    option_spec = {
        **CodeBlock.option_spec,
        "no-validate": directives.flag,
        "os": lambda value: directives.choice(value, tuple(specs.OS_OPTIONS)),
    }

    def run(self) -> list[nodes.Node]:
        if not self.arguments:
            self.arguments.append("console")
        if self.arguments[0] in lexers.LEXERS:
            self.arguments[0] = lexers.language(self.arguments[0])
        locations = [self.content.info(i) for i in range(len(self.content))]

        result = super().run()
        for block in list(result[0].findall(nodes.literal_block)):
            wrapper = nrfutil_command_block(
                validate="no-validate" not in self.options,
                platforms=self._platforms(),
                locations=[(source, offset + 1) for source, offset in locations],
            )
            if block.parent is None:
                result[0] = wrapper
            else:
                block.parent.replace(block, wrapper)
            wrapper += block
        return result

    def _platforms(self) -> list[str] | None:
        """The operating systems of the block, from the :os: option or the enclosing tab."""
        if "os" in self.options:
            return sorted(specs.OS_OPTIONS[self.options["os"]])
        # sphinx-tabs keeps the open tab sets in tabs_stack. The last title of each set is the tab
        # being parsed, because a title is added before the content of its tab is parsed.
        temp_data = self.env.temp_data
        for tabs_id in reversed(temp_data.get("tabs_stack", [])):
            titles = temp_data.get(f"tabs_{tabs_id}", {}).get("tab_titles", [])
            if titles and titles[-1][1].astext() in specs.TAB_PLATFORMS:
                return [specs.TAB_PLATFORMS[titles[-1][1].astext()]]
        return None


class _Location(nodes.Element):
    """A stand-in node that gives Sphinx warnings the file and line of a command."""


def _location(source: str, line: int) -> nodes.Element:
    location = _Location()
    location.source = source
    location.line = line
    return location


class NrfutilCommandTransform(SphinxTransform):
    """Expand substitutions, validate commands and add documentation links to the blocks.

    This runs after parsing because the substitutions from rst_epilog are only defined then.
    """

    default_priority = 210

    def apply(self, **kwargs: Any) -> None:
        for wrapper in list(self.document.findall(nrfutil_command_block)):
            self._process(wrapper)

    def _substitute(self, text: str, location: nodes.Element) -> str:
        def replace(match: re.Match) -> str:
            name = nodes.whitespace_normalize_name(match.group(1))
            definition = self.document.substitution_defs.get(name)
            if definition is None:
                # Like docutils, fall back to a case-insensitive match
                key = self.document.substitution_names.get(nodes.fully_normalize_name(name))
                definition = self.document.substitution_defs.get(key) if key else None
            if definition is None and name in SPHINX_CONFIG_SUBSTITUTIONS:
                return str(getattr(self.config, name, "") or match.group(0))
            if definition is None:
                logger.warning(
                    "Unknown substitution '|%s|' in nrfutil-command block",
                    name,
                    location=location,
                    type="nrfutil",
                    subtype="substitution",
                )
                return match.group(0)
            return definition.astext()

        return SUBSTITUTION_RE.sub(replace, text)

    def _process(self, wrapper: nrfutil_command_block) -> None:
        block = wrapper[0]
        locations = wrapper.attributes.pop("locations")
        platforms = wrapper.attributes.pop("platforms")
        validate = wrapper.attributes.pop("validate")
        default = locations[0] if locations else (block.source, block.line)

        def location(index: int) -> tuple[str, int]:
            return locations[index] if index < len(locations) else default

        text = "\n".join(
            self._substitute(line, _location(*location(i)))
            for i, line in enumerate(block.astext().split("\n"))
        )
        if text != block.astext():
            block.rawsource = text
            block.children = []
            block += nodes.Text(text)

        snapshot = _snapshot(*getattr(self.env, ENV_ATTRIBUTE, ("", 0)))
        if snapshot is None:
            return
        report = validate and (platforms is None or snapshot.platform in platforms)
        links = self._validate(snapshot, text, location, report)
        if links:
            wrapper["docs_url"] = next(iter(links.values()))

    def _validate(
        self, snapshot: specs.Snapshot, text: str, location, report: bool
    ) -> dict[str, str]:
        """Validate the commands in text and return the URLs of their documentation."""
        base_url, urls = specs.load_command_urls(self.config.nrfutil_command_urls)
        links: dict[str, str] = {}

        for index, command in iter_commands(text.split("\n")):
            try:
                context = validate_command(command, snapshot.core, snapshot.program)
            except ValidationError as e:
                if report:
                    logger.warning(
                        "%s\nnRF Util command: %s",
                        e,
                        command,
                        location=_location(*location(index)),
                        type="nrfutil",
                        subtype="command",
                    )
                if e.context is None:
                    continue
                context = e.context

            if self.config.nrfutil_command_links:
                path = context.subcommand_path()
                url = specs.command_url(base_url, urls, path)
                if url and url not in links.values():
                    links[" ".join(["nrfutil", *path])] = url
        return links


def _docs_button(url: str) -> str:
    """The button that opens the documentation of the first documented command in the block in a
    new tab, so that the reader keeps their place in the procedure.
    """
    return (
        f'<a class="{DOCS_BUTTON_CLASS} o-tooltip--left" href="{html.escape(url)}" '
        f'target="_blank" rel="noopener" aria-label="{html.escape(DOCS_LABEL)}" '
        f'data-tooltip="{html.escape(DOCS_TOOLTIP)}">{DOCS_ICON}</a>'
    )


def visit_block_html(translator, node: nrfutil_command_block) -> None:
    translator.body.append(f'<div class="{BLOCK_CLASS}">')


def depart_block_html(translator, node: nrfutil_command_block) -> None:
    if node.get("docs_url"):
        translator.body.append(_docs_button(node["docs_url"]))
    translator.body.append("</div>\n")


@functools.cache
def _snapshot(directory: str, _stamp: int) -> specs.Snapshot | None:
    """Load the snapshot. The stamp makes a regenerated snapshot load again in the same process."""
    if not directory:
        return None
    try:
        return specs.Snapshot.load(Path(directory))
    except ValidationError:
        return None


def _stamp(directory: Path) -> int:
    try:
        return (directory / specs.MANIFEST_FILE).stat().st_mtime_ns
    except OSError:
        return 0


def _no_snapshot(message: str) -> None:
    logger.warning(
        "nrfutil commands are not validated: %s", message, type="nrfutil", subtype="snapshot"
    )


def prepare_snapshot(app: Sphinx) -> None:
    """Generate the snapshot if it doesn't hold the pinned versions, and select it for the build.

    Without nrfutil_command_regenerate, the configured snapshot is used as it is.
    """
    setattr(app.env, ENV_ATTRIBUTE, ("", 0))
    if not app.config.nrfutil_command_check:
        return
    if not app.config.nrfutil_command_specs:
        _no_snapshot("nrfutil_command_specs is not set")
        return
    directory = Path(app.config.nrfutil_command_specs)

    if app.config.nrfutil_command_regenerate:
        platform = specs.host_platform()
        core_version = specs.load_core_version()
        pins = specs.load_pinned_versions()
        current = _snapshot(str(directory), _stamp(directory))

        if current is None or not current.matches(core_version, pins, platform):
            nrfutil = app.config.nrfutil_command_nrfutil
            if shutil.which(nrfutil) is None:
                _no_snapshot(
                    f"'{nrfutil}' was not found on the PATH, so the nrfutil command snapshot "
                    "can't be generated. Install nRF Util to enable the check"
                )
                return
            logger.info("Generating the nrfutil command snapshot in %s", directory)
            try:
                generate(directory, core_version, pins, platform, nrfutil)
            except SnapshotError as e:
                _no_snapshot(f"Failed to generate the nrfutil command snapshot: {e}")
                return

    stamp = _stamp(directory)
    if _snapshot(str(directory), stamp) is None:
        _no_snapshot(f"No nrfutil command snapshot in '{directory}'")
        return
    setattr(app.env, ENV_ATTRIBUTE, (str(directory), stamp))


def add_static_path(app: Sphinx, _config) -> None:
    app.config.html_static_path.append(STATIC_DIR.as_posix())


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("nrfutil_command_check", False, "env", types=bool)
    app.add_config_value("nrfutil_command_specs", "", "env")
    app.add_config_value("nrfutil_command_urls", str(specs.COMMAND_URLS_FILE), "env")
    app.add_config_value("nrfutil_command_links", True, "env")
    app.add_config_value("nrfutil_command_regenerate", True, "env", types=bool)
    app.add_config_value("nrfutil_command_nrfutil", "nrfutil", "")

    for name, lexer in lexers.LEXERS.items():
        app.add_lexer(lexers.language(name), lexer)
    app.add_node(nrfutil_command_block, html=(visit_block_html, depart_block_html))
    app.add_directive("nrfutil-command", NrfutilCommand)
    app.add_transform(NrfutilCommandTransform)
    app.connect("builder-inited", prepare_snapshot)
    app.connect("config-inited", add_static_path)
    app.add_css_file("nrfutil_command.css")

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

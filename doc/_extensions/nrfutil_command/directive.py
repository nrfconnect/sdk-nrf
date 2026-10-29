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
  block,
* with the ``:values:`` option, adds a switch that fills the placeholders with example values,
  which readers can edit before they copy the command, for example ``:values: version=|release|``.
  The switch starts on, unless the ``:example-toggle-off:`` option is set because the text
  explains the placeholders before the reader fills them,
* with the ``:values-windows:``, ``:values-linux:`` and ``:values-macos:`` options, adds
  operating system buttons that fill the placeholders with the values for that system.

Both forms of each command are validated: with placeholders and with the example values, for
each operating system that has values.

The snapshot is generated when the build starts if it doesn't hold the pinned versions. This needs
nrfutil on the PATH. Without it, the build prints a message and doesn't validate the commands,
unless ``nrfutil_command_required`` is set, in which case the build warns.

Validation problems are reported as Sphinx warnings at the file and line of the command.
Commands inside ``Windows``, ``Linux`` and ``macOS`` tabs are validated only if the snapshot comes
from that operating system, because the ``--json-args`` output differs between operating systems.
The ``:os:`` option sets the operating systems of a block instead, for example ``:os: windows``.
"""

from __future__ import annotations

import functools
import html
import json
import re
import shlex
import shutil
from pathlib import Path
from typing import Any

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.application import Sphinx
from sphinx.directives.code import CodeBlock
from sphinx.transforms import SphinxTransform
from sphinx.util import logging

from nrfutil_command import specs
from nrfutil_command.generate import SnapshotError, generate
from nrfutil_command.walker import ValidationError, iter_commands, validate_command

logger = logging.getLogger(__name__)

NODE_ATTRIBUTE = "nrfutil_command"
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
CONTROLS_CLASS = "nrfutil-command-controls"
FOOTER_CLASS = "nrfutil-command-footer"
STATIC_DIR = Path(__file__).parent / "static"
OS_VALUE_OPTIONS = {"values-windows": "windows", "values-linux": "linux", "values-macos": "macos"}

SUBSTITUTION_RE = re.compile(r"\|([^|\s][^|]*)\|")
# Sphinx resolves these substitutions from the configuration, not from substitution definitions
SPHINX_CONFIG_SUBSTITUTIONS = ("release", "version", "today")

PLACEHOLDER_RE = re.compile(r"<([^<>\s][^<>]*?)>")
PLACEHOLDER_HTML_RE = re.compile(r"&lt;([^&<>\s][^&<>]*?)&gt;")
DOCS_BUTTON = r'<a class="' + DOCS_BUTTON_CLASS + r'\b.*?</a>'
DOCS_BUTTON_RE = re.compile(r"(</pre>)(\s*</div>\s*</div>\s*)(" + DOCS_BUTTON + ")", re.DOTALL)
# The controls that follow the block, after its documentation button has moved into it
FOOTER_RE = re.compile(
    r"(</pre>(?:" + DOCS_BUTTON + r")?\s*</div>)(\s*</div>\s*)"
    r'(<div class="' + CONTROLS_CLASS + r'"[^>]*></div>)',
    re.DOTALL,
)
BLOCK_RE = re.compile(
    r'(<div class="[^"]*\b' + BLOCK_CLASS + r'\b[^"]*">)(.*?</pre>)'
    r"((?:" + DOCS_BUTTON + r")?\s*</div>(?:<div class=\"" + FOOTER_CLASS + r'">'
    r'<div class="' + CONTROLS_CLASS + r'" data-values="[^"]*" data-os-values="([^"]*)"[^>]*>'
    r"</div></div>)?)?",
    re.DOTALL,
)


def _values(argument: str | None) -> dict[str, str]:
    """Parse name=value pairs separated by spaces. Quote a pair that contains spaces."""
    values = {}
    for pair in shlex.split(argument or ""):
        name, equals, value = pair.partition("=")
        if not equals or not name:
            raise ValueError(f"expected name=value, got '{pair}'")
        values[name] = value
    return values


def _fill(text: str, values: dict[str, str]) -> str:
    """Replace the placeholders that have a value."""
    return PLACEHOLDER_RE.sub(lambda m: values.get(m.group(1), m.group(0)), text)


class NrfutilCommand(CodeBlock):
    """A code-block for nrfutil commands, validated during the documentation build."""

    required_arguments = 0
    optional_arguments = 1
    option_spec = {
        **CodeBlock.option_spec,
        "no-validate": directives.flag,
        "os": lambda value: directives.choice(value, tuple(specs.OS_OPTIONS)),
        "values": _values,
        **{option: _values for option in OS_VALUE_OPTIONS},
        "example-toggle-off": directives.flag,
    }

    def run(self) -> list[nodes.Node]:
        if not self.arguments:
            self.arguments.append("console")
        locations = [self.content.info(i) for i in range(len(self.content))]

        result = super().run()
        for block in result[0].findall(nodes.literal_block):
            block["classes"].append(BLOCK_CLASS)
            block[NODE_ATTRIBUTE] = {
                "validate": "no-validate" not in self.options,
                "platforms": self._platforms(),
                "locations": [(source, offset + 1) for source, offset in locations],
                "values": self.options.get("values", {}),
                "os_values": {
                    platform: self.options[option]
                    for option, platform in OS_VALUE_OPTIONS.items()
                    if option in self.options
                },
                "example_toggle_off": "example-toggle-off" in self.options,
            }
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
        for block in list(self.document.findall(nodes.literal_block)):
            data = block.get(NODE_ATTRIBUTE)
            if data is None:
                continue
            del block[NODE_ATTRIBUTE]
            self._process(block, data)

    def _substitute(self, text: str, location: nodes.Element) -> str:
        def replace(match: re.Match) -> str:
            name = match.group(1)
            definition = self.document.substitution_defs.get(name)
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

    def _process(self, block: nodes.literal_block, data: dict[str, Any]) -> None:
        locations = data["locations"]
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

        block_location = _location(*default)
        values = self._substitute_values(data["values"], block_location)
        os_values = {
            platform: self._substitute_values(platform_values, block_location)
            for platform, platform_values in data["os_values"].items()
        }
        self._check_values(text, values, os_values, block_location)
        if data["example_toggle_off"] and not values:
            logger.warning(
                "The :example-toggle-off: option has no effect without :values:",
                location=block_location,
                type="nrfutil",
                subtype="values",
            )

        links = self._validate_variants(text, values, os_values, data, location)
        # The documentation button must directly follow the block, so it goes in last
        if values or os_values:
            _insert_after(block, _controls(values, os_values, data["example_toggle_off"]))
        if links:
            _insert_after(block, _docs_button(links))

    def _validate_variants(
        self,
        text: str,
        values: dict[str, str],
        os_values: dict[str, dict[str, str]],
        data: dict[str, Any],
        location,
    ) -> dict[str, str]:
        """Validate the block for each operating system that has values, with the placeholders
        and with the example values, and return the URLs of the command documentation.
        """
        snapshot = _snapshot(*getattr(self.env, ENV_ATTRIBUTE, ("", 0)))
        if snapshot is None:
            return {}

        variants = [(platform, _fill(text, os_values[platform])) for platform in os_values]
        links: dict[str, str] = {}
        for platform, variant in variants or [(None, text)]:
            platforms = [platform] if platform else data["platforms"]
            report = data["validate"] and (platforms is None or snapshot.platform in platforms)
            for command, url in self._validate(snapshot, variant, location, report).items():
                if url not in links.values():
                    links[command] = url
            if values:
                self._validate(
                    snapshot, _fill(variant, values), location, report, "With the example values: "
                )
        return links

    def _substitute_values(self, values: dict[str, str], location: nodes.Element) -> dict[str, str]:
        return {name: self._substitute(value, location) for name, value in values.items()}

    def _check_values(
        self,
        text: str,
        values: dict[str, str],
        os_values: dict[str, dict[str, str]],
        location: nodes.Element,
    ) -> None:
        """Warn about values without a placeholder and about incomplete operating system values."""
        placeholders = set(PLACEHOLDER_RE.findall(text))
        os_names = {name for platform_values in os_values.values() for name in platform_values}

        def warn(message: str, *args: Any) -> None:
            logger.warning(message, *args, location=location, type="nrfutil", subtype="values")

        for name in sorted((set(values) | os_names) - placeholders):
            warn("Value for '<%s>', which isn't a placeholder in the nrfutil-command block", name)
        for name in sorted(set(values) & os_names):
            warn("Placeholder '<%s>' has both an example value and operating system values", name)
        for platform, platform_values in os_values.items():
            for name in sorted(os_names - set(platform_values)):
                warn("Placeholder '<%s>' has no value in :values-%s:", name, platform)

    def _validate(
        self,
        snapshot: specs.Snapshot,
        text: str,
        location,
        report: bool,
        prefix: str = "",
    ) -> dict[str, str]:
        """Validate the commands in text and return the URLs of their documentation."""
        base_url, urls = _command_urls(
            self.config.nrfutil_command_urls, self.config.nrfutil_command_docs_url
        )
        links: dict[str, str] = {}

        for index, command in iter_commands(text.split("\n")):
            try:
                context = validate_command(command, snapshot.core, snapshot.program)
            except ValidationError as e:
                if report:
                    logger.warning(
                        "%s%s\nnRF Util command: %s",
                        prefix,
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


def _docs_button(links: dict[str, str]) -> nodes.raw:
    """The button that opens the documentation of the first documented command in the block in a
    new tab, so that the reader keeps their place in the procedure. Only HTML builders render it.
    """
    url = html.escape(next(iter(links.values())))
    label = html.escape(DOCS_LABEL)
    markup = (
        f'<a class="{DOCS_BUTTON_CLASS} o-tooltip--left" href="{url}" target="_blank" '
        f'rel="noopener" '
        f'aria-label="{label}" data-tooltip="{html.escape(DOCS_TOOLTIP)}">{DOCS_ICON}</a>'
    )
    return nodes.raw("", markup, format="html")


def _controls(
    values: dict[str, str], os_values: dict[str, dict[str, str]], example_toggle_off: bool
) -> nodes.raw:
    """The element that nrfutil_command.js turns into the example values switch and the operating
    system buttons. Only HTML builders render it.
    """
    markup = (
        f'<div class="{CONTROLS_CLASS}" data-values="{html.escape(json.dumps(values))}" '
        f'data-os-values="{html.escape(json.dumps(os_values))}"'
        f'{" data-example-toggle-off" if example_toggle_off else ""}></div>'
    )
    return nodes.raw("", markup, format="html")


def _insert_after(block: nodes.literal_block, node: nodes.Node) -> None:
    """Insert the node after the block. rewrite_blocks moves it into the HTML block."""
    block.parent.insert(block.parent.index(block) + 1, node)


@functools.cache
def _snapshot(directory: str, _stamp: int) -> specs.Snapshot | None:
    """Load the snapshot. The stamp makes a regenerated snapshot load again in the same process."""
    if not directory:
        return None
    try:
        return specs.Snapshot.load(Path(directory))
    except ValidationError:
        return None


@functools.cache
def _command_urls(path: str, docs_url: str) -> tuple[str, dict[str, str]]:
    base_url, urls = specs.load_command_urls(Path(path))
    return docs_url or base_url, urls


def _stamp(directory: Path) -> int:
    try:
        return (directory / specs.MANIFEST_FILE).stat().st_mtime_ns
    except OSError:
        return 0


def _no_snapshot(app: Sphinx, message: str) -> None:
    if app.config.nrfutil_command_required:
        logger.warning(message, type="nrfutil", subtype="snapshot")
    else:
        # Not a warning, because local builds turn warnings into errors
        logger.info("Skipping the validation of nrfutil commands: %s.", message, color="yellow")


def prepare_snapshot(app: Sphinx) -> None:
    """Generate the snapshot if it doesn't hold the pinned versions, and select it for the build.

    Without nrfutil_command_ncs_root, the configured snapshot is used as it is.
    """
    setattr(app.env, ENV_ATTRIBUTE, ("", 0))
    if not app.config.nrfutil_command_specs:
        _no_snapshot(app, "nrfutil_command_specs is not set")
        return
    directory = Path(app.config.nrfutil_command_specs)
    root = app.config.nrfutil_command_ncs_root

    if root:
        platform = specs.host_platform()
        core_version = specs.load_core_version(Path(root), platform)
        pins = specs.load_pinned_versions(Path(root), platform)
        try:
            current = specs.Snapshot.load(directory)
        except ValidationError:
            current = None

        if current is None or not current.matches(core_version, pins, platform):
            nrfutil = app.config.nrfutil_command_nrfutil
            if shutil.which(nrfutil) is None:
                _no_snapshot(
                    app,
                    f"'{nrfutil}' was not found on the PATH, so the nrfutil command snapshot "
                    "can't be generated. Install nRF Util to enable the check",
                )
                return
            logger.info("Generating the nrfutil command snapshot in %s", directory)
            try:
                generate(directory, core_version, pins, platform, nrfutil)
            except SnapshotError as e:
                _no_snapshot(app, f"Failed to generate the nrfutil command snapshot: {e}")
                return

    stamp = _stamp(directory)
    if _snapshot(str(directory), stamp) is None:
        _no_snapshot(app, f"No nrfutil command snapshot in '{directory}'")
        return
    setattr(app.env, ENV_ATTRIBUTE, (str(directory), stamp))


def _move_footer(match: re.Match) -> str:
    return f'{match.group(1)}<div class="{FOOTER_CLASS}">{match.group(3)}</div>{match.group(2)}'


def _rewrite_block(match: re.Match) -> str:
    # Themes style code blocks with div[class^=highlight], so the highlight-<language> class
    # that Sphinx adds must stay first
    classes = match.group(1).split('"')[1].split()
    classes.remove(BLOCK_CLASS)
    opening = f'<div class="{" ".join([*classes, BLOCK_CLASS])}">'

    # Without JavaScript, the operating system placeholders show the values of the first system
    os_values = json.loads(html.unescape(match.group(4))) if match.group(4) else {}
    defaults = next(iter(os_values.values()), {})

    def placeholder(m: re.Match) -> str:
        name = html.unescape(m.group(1))
        attributes = f'data-name="{html.escape(name)}"'
        if name in defaults:
            return (
                f'<em class="nrfutil-placeholder nrfutil-os-value" {attributes}>'
                f"{html.escape(defaults[name])}</em>"
            )
        return f'<em class="nrfutil-placeholder" {attributes}>&lt;{m.group(1)}&gt;</em>'

    body = PLACEHOLDER_HTML_RE.sub(placeholder, match.group(2))
    return opening + body + (match.group(3) or "")


def rewrite_blocks(
    _app: Sphinx, _pagename: str, _templatename: str, context: dict, _doctree
) -> None:
    """Move the documentation button of each nrfutil-command block next to the copy button and
    the controls into the border of the block, and mark the placeholders, which are shown in
    italics.
    """
    body = context.get("body")
    if body and BLOCK_CLASS in body:
        body = DOCS_BUTTON_RE.sub(r"\1\3\2", body)
        body = FOOTER_RE.sub(_move_footer, body)
        context["body"] = BLOCK_RE.sub(_rewrite_block, body)


def add_static_path(app: Sphinx, _config) -> None:
    app.config.html_static_path.append(STATIC_DIR.as_posix())


def setup(app: Sphinx) -> dict[str, Any]:
    app.add_config_value("nrfutil_command_specs", "", "env")
    app.add_config_value("nrfutil_command_urls", str(specs.COMMAND_URLS_FILE), "env")
    app.add_config_value("nrfutil_command_docs_url", "", "env")
    app.add_config_value("nrfutil_command_links", True, "env")
    app.add_config_value("nrfutil_command_ncs_root", "", "env")
    app.add_config_value("nrfutil_command_nrfutil", "nrfutil", "")
    app.add_config_value("nrfutil_command_required", False, "")

    app.add_directive("nrfutil-command", NrfutilCommand)
    app.add_transform(NrfutilCommandTransform)
    app.connect("builder-inited", prepare_snapshot)
    app.connect("html-page-context", rewrite_blocks)
    app.connect("config-inited", add_static_path)
    app.add_css_file("nrfutil_command.css")
    app.add_js_file("nrfutil_command.js", loading_method="defer")

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }

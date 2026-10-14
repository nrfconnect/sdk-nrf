"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Validate the structure of ``nrfutil`` command examples in reStructuredText files.

Each example is split into shell tokens and walked against the ``--json-args`` output of nrfutil and
of the nrfutil commands it uses. The commands are installed on demand into a temporary
``NRFUTIL_HOME``, using the versions pinned in ``scripts/tools-versions-<os>.yml`` when available.
Any token that the walker cannot classify as a subcommand, flag, flag value or positional argument
is reported as an error.

The check is based on ``cargo xtask docs validate-commands`` from nrfutil-rs, but the rules here
are maintained independently and do not track the nrfutil-rs implementation.

Known limitations:

1. Missing trailing arguments or flag values are not detected.
2. Required flags are not checked.
3. Argument values are not validated, so placeholders such as ``<serial_number>`` are accepted.
4. ``--json-args`` does not describe variadic arguments or hidden aliases, so a second value for a
   variadic argument and aliases such as ``x-read`` are reported as errors.
5. Shell redirection, pipes, and command chaining (``>``, ``|``, ``;``, and similar) must be
   separated from neighboring tokens by whitespace (for example, ``list > out.json``, not
   ``list>out.json``). Operators glued to a word are kept as part of that token and may be reported
   as errors.

Marker comments placed before an example change how the next command is handled:

- ``.. validate command: skip next`` skips the next command.
- ``.. validate command: unix``, ``windows``, ``linux`` or ``macos`` validates the next command only
  on that platform.

Commands inside ``Windows``, ``Linux`` or ``macOS`` tabs are validated only on that platform,
unless a marker sets another one.
"""

from __future__ import annotations

import argparse
import fnmatch
import itertools
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
from collections.abc import Callable, Iterable, Iterator
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

NCS_ROOT = Path(__file__).resolve().parents[2]

DEFAULT_EXCLUDES = [
    "doc/nrf/releases_and_maturity/releases/**",
    "doc/nrf/releases_and_maturity/migration/**",
]

IGNORE_SUBCOMMANDS = {"+<modifier>", "+internal"}

# Directories whose .txt files are rst snippets that pages pull in with the include directive
INCLUDES_DIR = "includes"

SHELL_TERMINATORS = {">", ">>", "|", "||", "&&", ";"}

PROMPTS = ("$ ", "> ")

TOOLS_VERSIONS_FILES = {
    "linux": "tools-versions-linux.yml",
    "darwin": "tools-versions-darwin.yml",
    "win32": "tools-versions-win10.yml",
}

MARKER_RE = re.compile(r"^\.\.\s+validate command:\s*(?P<marker>.+?)\s*$")
TAB_RE = re.compile(r"^\.\.\s+(?:group-)?tab::\s*(?P<name>.+?)\s*$")
TAB_PLATFORMS = {"Windows": "windows", "Linux": "linux", "macOS": "macos"}
DIRECTIVE_RE = re.compile(r"^\.\.\s+(?P<name>code-block|code|sourcecode|parsed-literal)::")
OPTION_RE = re.compile(r"^:[\w-]+:")
LIST_MARKER_RE = re.compile(r"^(?:[-*+]|#\.|\d+\.|[a-zA-Z]\.|\(?\d+\))\s+")
SUBSTITUTION_RE = re.compile(r"\|[^|\s][^|]*\|")
ROLE_RE = re.compile(r":[\w-]+:`([^`]*)`")
EMPHASIS_RE = re.compile(r"\*{1,2}([^*\s][^*]*)\*{1,2}")
SHORTCUT_VERSION_RE = re.compile(
    r"^\.\.\s+\|\w*?nrfutil_(?P<command>[\w-]+)_ver\|\s+replace::\s*(?P<version>\S+)\s*$",
    re.MULTILINE,
)
PLACEHOLDER_RE = re.compile(r"<[^<>\s][^<>]*\s[^<>]*[^<>\s]>")

PLATFORMS = {
    "all": lambda: True,
    "unix": lambda: sys.platform.startswith("linux") or sys.platform == "darwin",
    "windows": lambda: sys.platform == "win32",
    "linux": lambda: sys.platform.startswith("linux"),
    "macos": lambda: sys.platform == "darwin",
}


class ValidationError(Exception):
    """An error in a single command example, or in the file at the given line."""

    def __init__(self, message: str, line: int | None = None):
        super().__init__(message)
        self.line = line


class SetupError(Exception):
    """An error in the environment that prevents any validation, such as a missing nrfutil."""


# --------------------------------------------------------------------------------------------------
# nrfutil runner
# --------------------------------------------------------------------------------------------------


def _load_tools_versions_nrfutil(ncs_root: Path) -> dict[str, Any]:
    name = TOOLS_VERSIONS_FILES.get(sys.platform, TOOLS_VERSIONS_FILES["linux"])
    path = ncs_root / "scripts" / name
    if not path.is_file():
        return {}
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    return data.get("nrfutil") or {}


def load_core_version(ncs_root: Path) -> str | None:
    """Return the nrfutil core version pinned in scripts/tools-versions-<os>.yml, if any."""
    version = _load_tools_versions_nrfutil(ncs_root).get("version")
    return str(version) if version else None


def load_pinned_versions(ncs_root: Path) -> dict[str, str]:
    """Return the nrfutil command versions to install, keyed by command.

    Versions in scripts/tools-versions-<os>.yml take precedence over the ``nrfutil_<command>_ver``
    substitutions in doc/nrf/shortcuts.txt, which document the versions used by the guides.
    """
    pins = {}

    shortcuts = ncs_root / "doc" / "nrf" / "shortcuts.txt"
    if shortcuts.is_file():
        for match in SHORTCUT_VERSION_RE.finditer(shortcuts.read_text(encoding="utf-8")):
            pins[match.group("command")] = match.group("version")

    subcommands = _load_tools_versions_nrfutil(ncs_root).get("subcommands") or {}
    pins.update({str(command): str(version) for command, version in subcommands.items()})

    return pins


class Nrfutil:
    """Runs nrfutil in a temporary NRFUTIL_HOME and caches installed commands and json-args.

    A new NRFUTIL_HOME bootstraps the latest nrfutil core, so a pinned core version is installed
    into it with ``self-upgrade`` before anything else runs.
    """

    def __init__(
        self,
        executable: str = "nrfutil",
        pinned_versions: dict[str, str] | None = None,
        core_version: str | None = None,
    ):
        self.executable = executable
        self.pinned_versions = pinned_versions or {}
        self.core_version = core_version
        self._home = tempfile.TemporaryDirectory(prefix="nrfutil-home-")
        self._programs: dict[str, dict[str, Any] | ValidationError] = {}

    def prepare(self) -> None:
        try:
            if self.core_version:
                print(f"Installing: nrfutil core {self.core_version}")
                self.run(["self-upgrade", "--to-version", self.core_version])
            print(self.run(["--version"]).splitlines()[0])
        except (ValidationError, IndexError) as e:
            raise SetupError(f"Unable to set up nrfutil: {e}") from e

    def close(self) -> None:
        self._home.cleanup()

    def run(self, args: list[str]) -> str:
        env = dict(os.environ, NRFUTIL_HOME=self._home.name)
        try:
            result = subprocess.run(
                [self.executable, *args],
                env=env,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
        except FileNotFoundError as e:
            raise SetupError(f"Unable to run '{self.executable}': {e}") from e

        if result.returncode != 0:
            output = result.stderr.strip() or result.stdout.strip()
            raise ValidationError(f"'nrfutil {' '.join(args)}' failed: {output}")
        return result.stdout

    def json_args(self) -> dict[str, Any]:
        try:
            stdout = self.run(["--json-args"])
        except ValidationError as e:
            raise SetupError(str(e)) from e

        try:
            return json.loads(stdout)
        except json.JSONDecodeError as e:
            raise SetupError(
                "Failed to parse the output of 'nrfutil --json-args'. "
                f"Try updating your installation of nrfutil. The output was: {stdout!r}"
            ) from e

    def program(self, subcommand_path: list[str]) -> dict[str, Any]:
        """Install the command at the given path if needed and return its json-args."""
        if len(subcommand_path) != 1:
            raise ValidationError(
                f"Unexpected subcommand path {subcommand_path}, expected exactly one command"
            )

        name = subcommand_path[0]
        if name not in self._programs:
            try:
                self._programs[name] = self._install_and_load(name)
            except ValidationError as e:
                self._programs[name] = e

        cached = self._programs[name]
        if isinstance(cached, ValidationError):
            raise cached
        return cached

    def _install_and_load(self, name: str) -> dict[str, Any]:
        version = self.pinned_versions.get(name)
        spec = f"{name}={version}" if version else name
        print(f"Installing: {spec}")
        try:
            self.run(["install", spec])
        except ValidationError as e:
            raise ValidationError(f"Could not install nrfutil command '{spec}': {e}") from e

        stdout = self.run([name, "--json-args"])
        try:
            return json.loads(stdout)
        except json.JSONDecodeError as e:
            raise ValidationError(
                f"Failed to parse the output of 'nrfutil {name} --json-args': {e}"
            ) from e


# --------------------------------------------------------------------------------------------------
# Argument walker
# --------------------------------------------------------------------------------------------------


def is_valid_partial(entry: Any, value: str) -> bool:
    """Whether value can be given for a value entry from --json-args."""
    if isinstance(entry, dict):
        if "possible" in entry:
            return any(possible.startswith(value) for possible in entry["possible"])
        if "optional" in entry:
            return is_valid_partial(entry["optional"], value)
    return True


def is_optional(entry: Any) -> bool:
    return isinstance(entry, dict) and "optional" in entry


class ValueCounter:
    """Tracks which of the values expected by a flag or a command have been given."""

    def __init__(self, entries: list[Any]):
        self.entries = entries
        self.index = 0

    @classmethod
    def for_command(cls, node: dict[str, Any]) -> ValueCounter:
        return cls([value for arg in node.get("arguments", []) for value in arg.get("values", [])])

    @classmethod
    def for_flag(cls, flag: dict[str, Any]) -> ValueCounter:
        return cls(list(flag.get("values", [])))

    def has_remaining(self) -> bool:
        return self.index < len(self.entries)

    def requires_value(self) -> bool:
        return any(not is_optional(entry) for entry in self.entries[self.index :])

    def take(self, value: str) -> bool:
        """Consume value if it fits the next entry, skipping optional entries it doesn't fit."""
        while self.index < len(self.entries):
            entry = self.entries[self.index]
            self.index += 1
            if not is_optional(entry) or is_valid_partial(entry, value):
                return True
        return False


@dataclass
class Stage:
    """A classified token or group of tokens on the command line.

    kind is one of: program, subcommand, external, flag, escape, escaped, unknown.
    """

    kind: str
    name: str = ""
    node: dict[str, Any] | None = None
    counter: ValueCounter | None = None
    values: list[str] = field(default_factory=list)

    def is_command(self) -> bool:
        return self.kind in ("program", "subcommand", "external")

    def take(self, value: str) -> bool:
        if self.counter is not None and self.counter.take(value):
            self.values.append(value)
            return True
        return False


ProgramProvider = Callable[[list[str]], dict[str, Any]]


class Context:
    def __init__(self, program: dict[str, Any]):
        self.stages = [Stage("program", node=program, counter=ValueCounter.for_command(program))]

    def push(self, stage: Stage) -> Stage:
        self.stages.append(stage)
        return stage

    def command(self) -> Stage:
        """The innermost command, subcommand or external command seen so far."""
        return next(stage for stage in reversed(self.stages) if stage.is_command())

    def find_flag(self, name: str, key: str) -> dict[str, Any] | None:
        """Find a flag on the current command or on any command it is nested in."""
        for stage in reversed(self.stages):
            if stage.is_command():
                for flag in stage.node.get("flags", []):
                    if flag.get(key) == name:
                        return flag
        return None

    def push_flag(self, name: str, flag: dict[str, Any]) -> Stage:
        return self.push(Stage("flag", name=name, node=flag, counter=ValueCounter.for_flag(flag)))

    def find_subcommand(self, name: str) -> dict[str, Any] | None:
        for subcommand in self.command().node.get("subcommands", []):
            if subcommand.get("name") == name:
                return subcommand
        return None

    def subcommand_path(self) -> list[str]:
        return [stage.name for stage in self.stages if stage.kind in ("subcommand", "external")]

    def allows_external_subcommand(self) -> bool:
        command = self.command()
        return command.kind == "program" and bool(command.node.get("allow_external_subcommands"))


def _walk_short_flags(context: Context, token: str) -> None:
    """Handle -a, -abc, -s VALUE, -sVALUE and -s=VALUE."""
    chars = token[1:]
    for i, short in enumerate(chars):
        flag = context.find_flag(short, "short")
        if flag is None:
            context.push(Stage("unknown", name=f"-{short}"))
            continue

        stage = context.push_flag(short, flag)
        rest = chars[i + 1 :]
        if rest and stage.counter.requires_value():
            stage.take(rest.removeprefix("="))
            return


def _walk_long_flag(context: Context, token: str) -> None:
    """Handle --flag, --flag VALUE and --flag=VALUE."""
    name, has_equals, value = token[2:].partition("=")
    flag = context.find_flag(name, "long")
    if flag is None:
        context.push(Stage("unknown", name=token))
        return

    stage = context.push_flag(name, flag)
    if has_equals and not stage.take(value):
        context.push(Stage("unknown", name=value))


def walk(tokens: list[str], program: dict[str, Any], provider: ProgramProvider) -> Context:
    """Classify every token after the command name.

    A token that follows a flag is taken as the flag's value if the flag requires one. Otherwise, it
    is tried, in order, as a subcommand, an optional value of the flag, an external nrfutil command
    and a positional argument of the current command. Anything else is unknown.
    """
    context = Context(program)
    escaped = False

    for token in tokens[1:]:
        if escaped:
            context.push(Stage("escaped", name=token))
            continue

        if token == "--":
            context.push(Stage("escape"))
            escaped = True
            continue

        last = context.stages[-1]
        pending_flag = last if last.kind == "flag" and last.counter.has_remaining() else None

        if pending_flag is not None and pending_flag.counter.requires_value():
            pending_flag.take(token)
            continue

        if token.startswith("--"):
            _walk_long_flag(context, token)
            continue

        if token.startswith("-") and token != "-":
            _walk_short_flags(context, token)
            continue

        subcommand = context.find_subcommand(token)
        if subcommand is not None:
            context.push(
                Stage(
                    "subcommand",
                    name=token,
                    node=subcommand,
                    counter=ValueCounter.for_command(subcommand),
                )
            )
            continue

        if pending_flag is not None and pending_flag.take(token):
            continue

        if context.allows_external_subcommand():
            external = provider([*context.subcommand_path(), token])
            context.push(
                Stage(
                    "external",
                    name=token,
                    node=external,
                    counter=ValueCounter.for_command(external),
                )
            )
            continue

        if context.command().take(token):
            continue

        context.push(Stage("unknown", name=token))

    return context


def tokenize(command: str) -> list[str]:
    """Split a command like a POSIX shell and drop any trailing redirection, pipes or chaining."""
    command = PLACEHOLDER_RE.sub(lambda m: re.sub(r"\s+", "_", m.group(0)), command)
    try:
        tokens = shlex.split(command)
    except ValueError as e:
        raise ValidationError(f"Failed to parse as shell command: {e}") from e

    stripped = []
    for token in tokens:
        if token in SHELL_TERMINATORS:
            break
        stripped.append(token)
    return stripped


def validate_command(command: str, program: dict[str, Any], provider: ProgramProvider) -> None:
    context = walk(tokenize(command), program, provider)
    for stage in context.stages:
        if stage.kind == "unknown":
            raise ValidationError(f"Found unrecognised element '{stage.name}'")


# --------------------------------------------------------------------------------------------------
# rst extraction
# --------------------------------------------------------------------------------------------------


@dataclass
class Example:
    path: Path
    line: int
    command: str
    skip: bool = False


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def _text_indent(line: str) -> int:
    """Indentation of the paragraph text, accounting for a leading list item marker."""
    stripped = line.lstrip(" ")
    match = LIST_MARKER_RE.match(stripped)
    return _indent(line) + (match.end() if match else 0)


def clean_parsed_literal(line: str) -> str:
    """Remove the inline markup that Sphinx resolves in parsed-literal blocks."""
    line = line.replace("\\ ", "")
    line = SUBSTITUTION_RE.sub("X", line)
    line = ROLE_RE.sub(r"\1", line)
    line = EMPHASIS_RE.sub(r"\1", line)
    return line


def _command_from_line(text: str) -> str | None:
    for prompt in PROMPTS:
        if text.startswith(prompt):
            text = text[len(prompt) :].lstrip()
            break
    if text.startswith("nrfutil "):
        return text.strip()
    return None


class RstExtractor:
    """Find nrfutil commands in literal blocks of an rst file."""

    def __init__(self, path: Path, content: str):
        self.path = path
        self.lines = content.expandtabs(8).splitlines()
        self.skip_next = False
        # Platform set by a marker for the next command, which overrides the enclosing tab
        self.platform: str | None = None
        # Enclosing Windows, Linux and macOS tabs, with the indentation of their directive
        self.tabs: list[tuple[int, str]] = []

    def examples(self) -> Iterator[Example]:
        i = 0
        while i < len(self.lines):
            line = self.lines[i]
            stripped = line.strip()

            if stripped:
                while self.tabs and self.tabs[-1][0] >= _indent(line):
                    self.tabs.pop()

            marker = MARKER_RE.match(stripped)
            if marker:
                self._apply_marker(marker.group("marker"), i)
                i += 1
                continue

            tab = TAB_RE.match(stripped)
            if tab:
                if tab.group("name") in TAB_PLATFORMS:
                    self.tabs.append((_indent(line), TAB_PLATFORMS[tab.group("name")]))
                i += 1
                continue

            directive = DIRECTIVE_RE.match(stripped)
            if directive:
                parsed = directive.group("name") == "parsed-literal"
                i = yield from self._block(i + 1, _indent(line), parsed, skip_options=True)
                continue

            if stripped.endswith("::") and not stripped.startswith(".."):
                i = yield from self._block(
                    i + 1, _text_indent(line), parsed=False, skip_options=False
                )
                continue

            i += 1

    def _apply_marker(self, marker: str, index: int) -> None:
        if marker == "skip next":
            self.skip_next = True
        elif marker in PLATFORMS:
            self.platform = marker
        else:
            raise ValidationError(f"Unknown validate command marker '{marker}'", line=index + 1)

    def _block(self, start: int, parent_indent: int, parsed: bool, skip_options: bool):
        """Yield the examples in the block body that starts at line index start, return the end."""
        i = start
        if skip_options:
            while (
                i < len(self.lines)
                and self.lines[i].strip()
                and _indent(self.lines[i]) > parent_indent
            ):
                if not OPTION_RE.match(self.lines[i].strip()):
                    break
                i += 1

        body: list[tuple[int, str]] = []
        while i < len(self.lines):
            line = self.lines[i]
            if line.strip() and _indent(line) <= parent_indent:
                break
            body.append((i, line))
            i += 1

        pending = ""
        pending_line = 0
        for index, line in body:
            text = line.strip()
            if parsed:
                text = clean_parsed_literal(text).strip()

            if pending:
                text = f"{pending} {text}"
            elif not text:
                continue
            else:
                pending_line = index

            if text.endswith("\\") and not text.endswith("\\\\"):
                pending = text[:-1].rstrip()
                continue
            pending = ""

            command = _command_from_line(text)
            if command is not None:
                yield self._example(pending_line, command)

        if pending:
            command = _command_from_line(pending)
            if command is not None:
                yield self._example(pending_line, command)

        return i

    def _example(self, index: int, command: str) -> Example:
        platform = self.platform or (self.tabs[-1][1] if self.tabs else "all")
        skip = self.skip_next or not PLATFORMS[platform]()
        self.skip_next = False
        self.platform = None
        return Example(self.path, index + 1, command, skip)


def extract_examples(path: Path, content: str) -> list[Example]:
    return list(RstExtractor(path, content).examples())


# --------------------------------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------------------------------


def _is_excluded(relative: str, excludes: Iterable[str]) -> bool:
    return any(fnmatch.fnmatch(relative, pattern) for pattern in excludes)


def is_rst_file(path: Path) -> bool:
    """Return whether the file contains rst: an rst file or a .txt file in an includes directory."""
    if path.suffix == ".rst":
        return True
    return path.suffix == ".txt" and path.parent.name == INCLUDES_DIR


def find_rst_files(root: Path, files: list[Path], excludes: list[str]) -> list[Path]:
    if files:
        candidates = [f if f.is_absolute() else Path.cwd() / f for f in files]
        candidates = [f for f in candidates if is_rst_file(f) and f.is_file()]
    else:
        candidates = [
            p
            for p in itertools.chain(root.rglob("*.rst"), root.rglob("*.txt"))
            if is_rst_file(p)
            and not any(part.startswith(("_build", ".")) for part in p.relative_to(root).parts)
        ]

    result = []
    for path in sorted(candidates):
        try:
            relative = path.resolve().relative_to(root.resolve()).as_posix()
        except ValueError:
            relative = path.as_posix()
        if not _is_excluded(relative, excludes):
            result.append(path)
    return result


def _escape_annotation(value: str, is_property: bool = False) -> str:
    value = value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    if is_property:
        value = value.replace(":", "%3A").replace(",", "%2C")
    return value


def report_error(message: str, file: str | None = None, line: int | None = None) -> None:
    """Print an error and, in GitHub Actions, annotate it on the file and line it refers to."""
    location = f"{file}:{line}: " if file and line else f"{file}: " if file else ""
    print(f"ERROR: {location}{message}")

    if os.environ.get("GITHUB_ACTIONS") != "true":
        return
    properties = ["title=nRF Util command check"]
    if file:
        properties.append(f"file={_escape_annotation(file, is_property=True)}")
    if line:
        properties.append(f"line={line}")
    print(f"::error {','.join(properties)}::{_escape_annotation(message)}")


def validate_files(paths: list[Path], root: Path, nrfutil: Nrfutil) -> int:
    nrfutil.prepare()
    program = nrfutil.json_args()
    errors = 0

    for path in paths:
        try:
            relative = path.resolve().relative_to(root.resolve()).as_posix()
        except ValueError:
            relative = str(path)

        try:
            examples = extract_examples(Path(relative), path.read_text(encoding="utf-8"))
        except ValidationError as e:
            report_error(str(e), relative, e.line)
            errors += 1
            continue

        for example in examples:
            if example.skip:
                continue
            tokens = example.command.split()
            if len(tokens) > 1 and tokens[1] in IGNORE_SUBCOMMANDS:
                continue

            print(f"Validating: {example.command}")
            try:
                validate_command(example.command, program, nrfutil.program)
            except ValidationError as e:
                report_error(f"{e}\nCommand: {example.command}", relative, example.line)
                errors += 1

    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog=__file__,
        description="Validate the structure of nrfutil command examples in rst files",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=NCS_ROOT,
        help="Root directory to search for rst files and scripts/tools-versions-*.yml",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        default=None,
        metavar="GLOB",
        help="Glob, relative to the root, of files to skip. Can be repeated. "
        f"Default: {', '.join(DEFAULT_EXCLUDES)}",
    )
    parser.add_argument("--nrfutil", default="nrfutil", help="nrfutil executable to use")
    parser.add_argument(
        "--pin",
        action="append",
        default=[],
        metavar="COMMAND=VERSION",
        help="Version of an nrfutil command to install, overriding the versions found in "
        "scripts/tools-versions-<os>.yml and doc/nrf/shortcuts.txt. Can be repeated.",
    )
    parser.add_argument(
        "--latest",
        action="store_true",
        help="Use the latest nrfutil core and commands, ignoring all pinned versions",
    )
    parser.add_argument(
        "files", nargs="*", type=Path, help="rst files to check instead of all files under the root"
    )
    args = parser.parse_args(argv)

    excludes = DEFAULT_EXCLUDES if args.exclude is None else args.exclude
    paths = find_rst_files(args.root, args.files, excludes)

    if args.latest and args.pin:
        parser.error("--latest and --pin cannot be used together")

    pins = {} if args.latest else load_pinned_versions(args.root)
    for pin in args.pin:
        command, sep, version = pin.partition("=")
        if not sep or not command or not version:
            parser.error(f"--pin must have the form COMMAND=VERSION, got '{pin}'")
        pins[command] = version

    core_version = None if args.latest else load_core_version(args.root)
    nrfutil = Nrfutil(args.nrfutil, pins, core_version)
    try:
        errors = validate_files(paths, args.root, nrfutil)
    except SetupError as e:
        report_error(str(e))
        return 2
    finally:
        nrfutil.close()

    if errors:
        print(f"\nCommand validation failed: {errors} error(s) in {len(paths)} file(s) checked")
        return 1

    print(f"\nCommand validation passed: {len(paths)} file(s) checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())

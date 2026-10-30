"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Walk ``nrfutil`` command lines against the ``--json-args`` output of nrfutil and its commands.

Each command is split into shell tokens and every token is classified as a subcommand, flag, flag
value or positional argument. Anything the walker cannot classify is reported as an error.

The rules are based on ``cargo xtask docs validate-commands`` from nrfutil-rs, but they are
maintained independently and do not track the nrfutil-rs implementation.
"""

from __future__ import annotations

import re
import shlex
from collections.abc import Callable, Iterable, Iterator
from dataclasses import dataclass, field
from typing import Any

SHELL_TERMINATORS = {">", ">>", "|", "||", "&&", ";"}

PROMPTS = ("$ ", "> ")

# Placeholders that contain whitespace, such as <hex file>, which must stay a single token
SPACED_PLACEHOLDER_RE = re.compile(r"<[^<>\s][^<>]*\s[^<>]*[^<>\s]>")


class ValidationError(Exception):
    """An error in a single command example, or in the file at the given line."""

    def __init__(self, message: str, line: int | None = None, context: Context | None = None):
        super().__init__(message)
        self.line = line
        # The walk of the command up to the error, which still gives its subcommand path
        self.context = context


def is_placeholder(value: str) -> bool:
    """Whether value is a placeholder such as <serial_number>, which stands for any value."""
    return len(value) > 2 and value.startswith("<") and value.endswith(">")


def is_valid_partial(entry: Any, value: str) -> bool:
    """Whether value can be given for a value entry from --json-args."""
    if is_placeholder(value):
        return True
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
    command = SPACED_PLACEHOLDER_RE.sub(lambda m: re.sub(r"\s+", "_", m.group(0)), command)
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


def validate_command(command: str, program: dict[str, Any], provider: ProgramProvider) -> Context:
    """Walk the command and raise ValidationError for the first token that isn't recognised."""
    context = walk(tokenize(command), program, provider)
    for stage in context.stages:
        if stage.kind == "unknown":
            raise ValidationError(
                f"Found unrecognized syntax element '{stage.name}'", context=context
            )
    return context


def command_from_line(text: str) -> str | None:
    """Return the nrfutil command on the line, without any $ or > prompt, or None."""
    for prompt in PROMPTS:
        if text.startswith(prompt):
            text = text[len(prompt) :].lstrip()
            break
    if text.startswith("nrfutil "):
        return text.strip()
    return None


def iter_commands(lines: Iterable[str]) -> Iterator[tuple[int, str]]:
    """Yield the index of the first line and the text of each nrfutil command in a literal block.

    Lines that end with a backslash are joined with the next line.
    """
    pending = ""
    pending_index = 0
    for index, line in enumerate(lines):
        text = line.strip()
        if pending:
            text = f"{pending} {text}"
        elif not text:
            continue
        else:
            pending_index = index

        if text.endswith("\\") and not text.endswith("\\\\"):
            pending = text[:-1].rstrip()
            continue
        pending = ""

        command = command_from_line(text)
        if command is not None:
            yield pending_index, command

    if pending:
        command = command_from_line(pending)
        if command is not None:
            yield pending_index, command

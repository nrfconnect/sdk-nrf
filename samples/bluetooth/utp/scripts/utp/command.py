#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""Common base class of the commands that can be run from the shell."""

import inspect
from abc import ABC, abstractmethod
from enum import IntEnum
from typing import Any, NamedTuple

import serial


class Parameter(NamedTuple):
    """A constructor parameter of a command."""

    name: str
    type: type
    default: Any


class Command(ABC):
    """Base class of all commands. Concrete commands in any module of the utp package are
    discovered by the shell.

    A module that defines commands sets a module level CATEGORY, the title of the group
    the commands are listed under.
    """

    opcode: int
    name: str

    @abstractmethod
    def help(self) -> str:
        """Human readable description of the command and its parameters."""

    @abstractmethod
    def run(self, port: serial.Serial):
        """Send the command over the serial port and print the result."""

    @classmethod
    def parameters(cls) -> list[Parameter]:
        """Return the constructor parameters of a command class as Parameter tuples.

        The type is the annotation of the parameter, for example int, bool, bytes or an
        IntEnum. The default is None for a required parameter.
        """
        params = list(inspect.signature(cls.__init__).parameters.values())[1:]
        # A class without its own __init__ has (*args, **kwargs) and takes no parameters
        return [
            Parameter(
                p.name,
                p.annotation,
                p.default if p.default is not inspect.Parameter.empty else None,
            )
            for p in params
            if p.kind not in (p.VAR_POSITIONAL, p.VAR_KEYWORD)
        ]

    @classmethod
    def parameter_choices(cls, index: int) -> list[str]:
        """Return the acceptable values of parameter `index` of `cls`, or an empty list."""
        params = cls.parameters()
        if index >= len(params):
            return []
        param_type = params[index].type
        if param_type is bool:
            return ["true", "false"]
        if issubclass(param_type, IntEnum):
            return [m.name for m in param_type]
        return []

#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
SDFW ADAC CLI.
"""

from __future__ import annotations

import argparse
import dataclasses
import inspect
import re
import typing
from typing import Any, Dict

import sdfw_adac_cmd
from ctrlap import Ctrlap
from sdfw_adac import Adac


def main() -> None:
    args = parse_arguments()
    do_adac_transaction(**vars(args))


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send SDFW ADAC commands over CTRL-AP using JLink",
        allow_abbrev=False,
    )

    Ctrlap.add_arguments(parser)

    subparsers = parser.add_subparsers(
        title="ADAC commands", description="Supported SDFW ADAC commands", required=True
    )

    for request_type in [
        sdfw_adac_cmd.Version,
        sdfw_adac_cmd.MemCfg,
        sdfw_adac_cmd.Revert,
        sdfw_adac_cmd.Reset,
        sdfw_adac_cmd.MemErase,
        sdfw_adac_cmd.LcsGet,
        sdfw_adac_cmd.LcsSet,
        sdfw_adac_cmd.Ssf,
        sdfw_adac_cmd.Purge,
    ]:
        add_request_subcommand(subparsers, request_type)

    return parser.parse_args()


def add_request_subcommand(subparsers: Any, request_type: type) -> None:
    """Add a subcommand for doing an ADAC transaction with the given request type.
    This relies on dataclass introspection/type hints and supports simple dataclasses without
    nesting and fields with either int, bytes, bool or string types.
    Each subcommand automatically sets 'request_type' in the parsed arguments to the given
    request type class.
    """

    command_name = to_kebab_case(request_type.__name__)
    command_help = inspect.getdoc(request_type)

    command_parser = subparsers.add_parser(command_name, help=command_help)
    command_parser.set_defaults(request_type=request_type)

    type_hints = typing.get_type_hints(request_type)

    for field in dataclasses.fields(request_type):
        arg_name = f"--{to_kebab_case(field.name)}"
        default_value = (
            field.default
            if field.default != dataclasses.MISSING
            else field.default_factory
        )
        is_required = default_value == dataclasses.MISSING

        field_type = type_hints[field.name]
        type_kwargs: Dict[str, Any] = {}

        if issubclass(int, field_type):
            type_kwargs["type"] = lambda x: int(x, 0)
        elif issubclass(bytes, field_type):
            type_kwargs["type"] = bytes.fromhex
        elif issubclass(bool, field_type):
            type_kwargs["action"] = "store_true"
            is_required = False
        elif not issubclass(str, field_type):
            raise ValueError(
                f"Unsupported field type for {request_type.__name__}.{field.name}: {field_type}"
            )

        if default_value != dataclasses.MISSING:
            type_kwargs["default"] = default_value

        command_parser.add_argument(
            arg_name,
            dest=field.name,
            required=is_required,
            **type_kwargs,
        )


def do_adac_transaction(request_type: type, **kwargs: Any) -> None:
    """Perform a single ADAC transaction using the given request type."""

    request_fields = {f.name for f in dataclasses.fields(request_type)}
    request_kwargs = {k: v for k, v in kwargs.items() if k in request_fields}
    request = request_type(**request_kwargs)

    ctrlap = Ctrlap(**kwargs)
    adac = Adac(ctrlap)

    adac.ctrlap.connector.connect()

    print(f"--> {request}")

    response = adac.request(request.to_request())

    print(f"<-- {response}")

    adac.ctrlap.connector.disconnect()


def to_kebab_case(string: str) -> str:
    """Convert the given string to kebab case (lowercase-with-hyphens)."""

    string = re.sub("([a-z0-9])([A-Z])", r"\1-\2", string)
    string = string.replace("_", "-").lower()
    return string


if __name__ == "__main__":
    main()

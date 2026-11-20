#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""Interactive shell for a board running the utp sample (UTP and Channel Sounding tests).

Usage: python3 utp_shell.py [port] [--baud 1000000]

Without a port, the connected devices are listed with `nrfutil device list` and one can be
selected. Type `?` for the list of commands, `help <command>` for the help of a command.
"""

import argparse
import cmd
import importlib
import inspect
import json
import pkgutil
import re
import readline
import shlex
import shutil
import struct
import subprocess
import sys
from enum import IntEnum
from types import ModuleType
from typing import Any, TextIO

import serial
import utp
from utp.command import Command
from utp.transport import hci_reset


def snake_case(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).lower()


def find_command_modules() -> list[ModuleType]:
    """Import every module of the utp package that defines a CATEGORY of commands."""
    modules = (
        importlib.import_module(f"{utp.__name__}.{info.name}")
        for info in pkgutil.iter_modules(utp.__path__)
    )
    return [m for m in modules if hasattr(m, "CATEGORY")]


def find_command_classes() -> dict[str, type[Command]]:
    """Map the shell name of each command to its class."""
    commands = {}
    for module in find_command_modules():
        for _, cls in inspect.getmembers(module, inspect.isclass):
            if issubclass(cls, Command) and not inspect.isabstract(cls):
                command_name = snake_case(cls.__name__)
                commands[command_name] = cls
    return commands


def parse_bool(text: str) -> bool:
    if text.lower() in ("1", "true", "yes", "on"):
        return True
    if text.lower() in ("0", "false", "no", "off"):
        return False
    raise ValueError(f"'{text}' is not true or false")


def parse_argument(cls: type[Command], index: int, text: str) -> Any:
    param_type = cls.parameters()[index].type
    if param_type is bool:
        return parse_bool(text)
    if param_type is bytes:
        return bytes.fromhex(text)
    if issubclass(param_type, IntEnum) and text.upper() in param_type.__members__:
        return param_type[text.upper()]
    return int(text, 0)


def select_port() -> str:
    """Let the user pick a serial port from `nrfutil device list`."""
    if not shutil.which("nrfutil"):
        sys.exit("No port given and nrfutil was not found")
    print("No port given, listing connected devices with 'nrfutil device list'...")
    result = subprocess.run(
        ["nrfutil", "device", "list", "--json"], capture_output=True, text=True, check=False
    )
    ports = []
    for line in result.stdout.splitlines():
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            continue
        if msg.get("type") != "info":
            continue
        for dev in msg["data"].get("devices", []):
            for sp in dev.get("serialPorts", []):
                ports.append((dev["serialNumber"], sp["vcom"], sp["comName"]))
    if not ports:
        sys.exit("No devices with serial ports found")
    if len(ports) == 1:
        print(f"Found one device (serial {ports[0][0]}), using its port {ports[0][2]}")
        return ports[0][2]
    print("Found several serial ports.")
    for i, (serial_number, vcom, com) in enumerate(ports):
        print(f"  [{i}] {com}  (device {serial_number}, vcom {vcom})")
    while True:
        choice = input("Select port: ").strip()
        if choice.isdigit() and int(choice) < len(ports):
            return ports[int(choice)][2]


class UtpShell(cmd.Cmd):
    """The shell. A do_/complete_/help_ method is added per command class on creation."""

    prompt = "utp> "
    intro: str | None = (
        "Controller ready. Type ? for the list of commands, 'help <command>' for the help of a "
        "command, Tab to complete."
    )

    def __init__(self, port: serial.Serial, log_file: TextIO | None = None):
        super().__init__()
        self.port = port
        self.executed: list[str] = []
        self.log_file = log_file

        self.commands = find_command_classes()
        self.categories = {
            module.CATEGORY: sorted(
                n for n, c in self.commands.items() if c.__module__ == module.__name__
            )
            for module in find_command_modules()
        }

        for name, cls in self.commands.items():
            # Default arguments bind the current name and class (a closure would see the last).
            setattr(self, f"do_{name}", lambda arg, name=name: self.run_command(name, arg))
            setattr(self, f"help_{name}", lambda name=name: self.print_command_help(name))
            setattr(
                self,
                f"complete_{name}",
                lambda text, line, begidx, endidx, cls=cls: self.command_complete(
                    cls, text, line, begidx
                ),
            )

    def print_command_help(self, name: str) -> None:
        cls = self.commands[name]
        params = cls.parameters()
        usage = " ".join(
            f"<{p.name}>" if p.default is None else f"[{p.name}={p.default}]" for p in params
        )
        print(f"{cls.name}")
        print(f"  Usage: {name} {usage}".rstrip())
        for i, param in enumerate(params):
            param_choices = cls.parameter_choices(i)
            if len(param_choices) > 0:
                print(f"  {param.name}: " + ", ".join(param_choices))
        print("  Parameters can be given by position or by name, e.g. 'role=reflector'.")
        print("  Numbers can be given in decimal or as 0x... hex.")

    def command_complete(self, cls: type[Command], text: str, line: str, begidx: int) -> list[str]:
        key, sep, value = text.partition("=")
        names = [p.name for p in cls.parameters()]
        if sep and key in names:
            choices = cls.parameter_choices(names.index(key))
            return [f"{key}={c}" for c in choices if c.lower().startswith(value.lower())]
        index = len(line[:begidx].split()) - 1
        choices = cls.parameter_choices(index) if index < len(names) else []
        choices += [f"{p}=" for p in names]
        return [c for c in choices if c.lower().startswith(text.lower())]

    def build_command(self, name: str, args: list[str]) -> Command:
        """Create a command from shell arguments: positional values and/or parameter=value."""
        cls = self.commands[name]
        params = cls.parameters()
        names = [p.name for p in params]
        values = {}
        positional = 0
        for arg in args:
            key, sep, text = arg.partition("=")
            if sep and key in names:
                index = names.index(key)
            elif sep and key.isidentifier():
                raise ValueError(f"unknown parameter '{key}', see 'help {name}'")
            else:
                index, text = positional, arg
                positional += 1
                if index >= len(params):
                    raise ValueError(f"too many arguments, see 'help {name}'")
            if names[index] in values:
                raise ValueError(f"parameter '{names[index]}' given more than once")
            values[names[index]] = parse_argument(cls, index, text)
        missing = [p.name for p in params if p.default is None and p.name not in values]
        if missing:
            raise ValueError(f"missing parameter(s): {', '.join(missing)}, see 'help {name}'")
        return cls(**values)

    def get_names(self) -> list[str]:
        # cmd only looks at the class; include the per-instance command methods
        return list(dict.fromkeys(super().get_names() + dir(self)))

    def onecmd(self, line: str) -> bool:
        try:
            stop = super().onecmd(line)
            word = line.split()[0] if line.split() else ""
            if word in self.commands:
                self.executed.append(line.strip())
                if self.log_file:
                    # flushed per command so the log survives a crash and can be replayed
                    self.log_file.write(line.strip() + "\n")
                    self.log_file.flush()
            return stop
        except (ValueError, RuntimeError, serial.SerialException, struct.error) as e:
            print(f"Error: {e}")
        except SystemExit as e:
            # the transport exits on timeouts and HCI errors; keep the shell alive
            print(f"Error: {e}")
        return False

    def preloop(self) -> None:
        # '=' must not split a word, so that parameter=value can be completed
        readline.set_completer_delims(" \t\n")

    def emptyline(self) -> bool:
        # By default, cmd repeats the last command, which would resend a test command
        return False

    def default(self, line: str) -> None:
        print(f"Unknown command '{line.split()[0]}'. Type ? for the list of commands.")

    def do_help(self, arg: str) -> bool | None:
        """List the commands by category, or show the help of one command"""
        if arg:
            return super().do_help(arg)
        for title, names in self.categories.items():
            self.print_topics(f"{title} (type help <command>)", names, 15, 80)
        self.print_topics("Shell commands", ["exit", "help"], 15, 80)
        return False

    def do_exit(self, _arg: str) -> bool:
        """Leave the shell"""
        return True

    do_EOF = do_exit

    def run_command(self, name: str, arg: str) -> None:
        self.build_command(name, shlex.split(arg)).run(self.port)


def run_shell(port: serial.Serial, log_file: TextIO | None = None) -> None:
    shell = UtpShell(port, log_file)
    while True:
        try:
            shell.cmdloop()
            break
        except KeyboardInterrupt:
            print()
            shell.intro = None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("port", nargs="?", help="Serial port, e.g. /dev/ttyACM1")
    parser.add_argument("--baud", type=int, default=1000000)
    parser.add_argument(
        "--log",
        metavar="FILE",
        help="Log every executed test command to FILE (replayable with run_script.py)",
    )
    args = parser.parse_args()

    port_name = args.port or select_port()

    print(f"Opening {port_name} at {args.baud} baud with hardware flow control...")
    with serial.Serial(port_name, args.baud, rtscts=True, timeout=5) as port:
        print("Initializing the controller.")
        hci_reset(port)

        if args.log:
            with open(args.log, "w", encoding="utf-8") as log_file:
                run_shell(port, log_file)
        else:
            run_shell(port)


if __name__ == "__main__":
    main()

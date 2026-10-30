"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "_extensions"))

from nrfutil_command import walker as vnc  # noqa: E402

UNKNOWN = {"type": "UNKNOWN"}


def flag(long=None, short=None, values=()):
    return {"name": long or short, "long": long, "short": short, "values": list(values)}


TEST_FLAGS = [
    flag(short="t"),
    flag(long="test-long"),
    flag(long="test-long-value", values=[UNKNOWN]),
    flag(long="test-long-multiple-value", values=[UNKNOWN, UNKNOWN]),
    flag(long="test-long-optional-value", values=[{"optional": UNKNOWN}]),
    flag(long="test-long-optional-value-multiple-value", values=[UNKNOWN, {"optional": UNKNOWN}]),
]

TEST_PROGRAM = {
    "allow_external_subcommands": False,
    "flags": TEST_FLAGS,
    "arguments": [
        {"name": "arg1", "values": [UNKNOWN]},
        {"name": "arg2", "values": [UNKNOWN]},
    ],
    "subcommands": [
        {
            "name": "subcommand",
            "arguments": [{"name": "package", "values": [{"type": "FILE"}]}],
            "flags": TEST_FLAGS,
            "subcommands": [
                {
                    "name": "subsubcommand",
                    "arguments": [],
                    "flags": [flag(short="s")],
                    "subcommands": [],
                }
            ],
        }
    ],
}

NRFUTIL_PROGRAM = {
    "allow_external_subcommands": True,
    "flags": [
        flag(long="log-level", values=[{"possible": ["off", "info"]}]),
        flag(long="version", short="V"),
    ],
    "arguments": [{"name": "modifier", "values": [{"optional": {"possible": ["+internal"]}}]}],
    "subcommands": [
        {
            "name": "install",
            "flags": [flag(long="force", short="f")],
            "arguments": [{"name": "subcommands", "values": [{"optional": UNKNOWN}]}],
            "subcommands": [],
        }
    ],
}

DEVICE_PROGRAM = {
    "allow_external_subcommands": False,
    "flags": [flag(long="version", short="V")],
    "arguments": [],
    "subcommands": [
        {
            "name": "program",
            "flags": [
                flag(long="firmware", values=[{"type": "PATH"}]),
                flag(long="serial-number", values=[{"type": "SERIAL_NUMBERS"}]),
                flag(long="traits", values=[{"comma-array-possible": ["jlink"]}]),
            ],
            "arguments": [],
            "subcommands": [],
        },
        {"name": "list", "flags": [], "arguments": [], "subcommands": []},
    ],
}


def panic_provider(path):
    raise AssertionError(f"Unexpected external subcommand lookup: {path}")


def device_provider(path):
    if path == ["device"]:
        return DEVICE_PROGRAM
    raise vnc.ValidationError(f"Could not install nrfutil command '{path[0]}'")


def kinds(context):
    return [(stage.kind, stage.name, stage.values) for stage in context.stages]


def test_walker_comprehensive_command():
    tokens = [
        "nrfutil",
        "--test-long",
        "--test-long-value",
        "test-long-value",
        "--test-long-multiple-value",
        "test-long-multiple-value1",
        "test-long-multiple-value2",
        "arg1",
        "--test-long-optional-value",
        "test-long-optional-value",
        "--test-long-optional-value-multiple-value",
        "test-long-optional-value-multiple-value1",
        "test-long-optional-value-multiple-value2",
        "arg2",
        "subcommand",
        "--test-long",
        "arg1",
        "subsubcommand",
    ]

    context = vnc.walk(tokens, TEST_PROGRAM, panic_provider)

    assert kinds(context) == [
        ("program", "", ["arg1", "arg2"]),
        ("flag", "test-long", []),
        ("flag", "test-long-value", ["test-long-value"]),
        (
            "flag",
            "test-long-multiple-value",
            ["test-long-multiple-value1", "test-long-multiple-value2"],
        ),
        ("flag", "test-long-optional-value", ["test-long-optional-value"]),
        (
            "flag",
            "test-long-optional-value-multiple-value",
            [
                "test-long-optional-value-multiple-value1",
                "test-long-optional-value-multiple-value2",
            ],
        ),
        ("subcommand", "subcommand", ["arg1"]),
        ("flag", "test-long", []),
        ("subcommand", "subsubcommand", []),
    ]


def test_walker_unknown_flag_and_extra_argument():
    context = vnc.walk(["nrfutil", "--nope", "a", "b", "c"], TEST_PROGRAM, panic_provider)
    assert [s.name for s in context.stages if s.kind == "unknown"] == ["--nope", "c"]


def test_walker_short_flags():
    context = vnc.walk(
        ["nrfutil", "subcommand", "subsubcommand", "-st"], TEST_PROGRAM, panic_provider
    )
    assert kinds(context)[-2:] == [("flag", "s", []), ("flag", "t", [])]

    context = vnc.walk(["nrfutil", "-x"], TEST_PROGRAM, panic_provider)
    assert kinds(context)[-1] == ("unknown", "-x", [])


@pytest.mark.parametrize(
    "tokens",
    [["-v", "value"], ["-vvalue"], ["-v=value"], ["-tv", "value"], ["-tvvalue"]],
)
def test_walker_short_flag_values(tokens):
    program = dict(TEST_PROGRAM, flags=[flag(short="t"), flag(short="v", values=[UNKNOWN])])
    program["arguments"] = []
    context = vnc.walk(["nrfutil", *tokens], program, panic_provider)
    assert kinds(context)[-1] == ("flag", "v", ["value"])
    assert not [s for s in context.stages if s.kind == "unknown"]


def test_walker_long_flag_with_equals_consumes_value():
    context = vnc.walk(["nrfutil", "--test-long-value=x", "arg1"], TEST_PROGRAM, panic_provider)
    assert kinds(context) == [
        ("program", "", ["arg1"]),
        ("flag", "test-long-value", ["x"]),
    ]

    context = vnc.walk(["nrfutil", "--test-long=x"], TEST_PROGRAM, panic_provider)
    assert kinds(context)[-1] == ("unknown", "x", [])


def test_walker_prefers_subcommand_over_optional_flag_value():
    tokens = ["nrfutil", "--test-long-optional-value", "subcommand", "file.zip"]
    context = vnc.walk(tokens, TEST_PROGRAM, panic_provider)
    assert kinds(context)[1:] == [
        ("flag", "test-long-optional-value", []),
        ("subcommand", "subcommand", ["file.zip"]),
    ]


def test_walker_optional_argument_is_skipped_when_value_does_not_fit():
    program = dict(TEST_PROGRAM, subcommands=[])
    program["arguments"] = [
        {"name": "modifier", "values": [{"optional": {"possible": ["+internal"]}}]},
        {"name": "path", "values": [UNKNOWN]},
    ]
    context = vnc.walk(["nrfutil", "some/path"], program, panic_provider)
    assert kinds(context) == [("program", "", ["some/path"])]


def test_walker_escape():
    context = vnc.walk(["nrfutil", "--", "--anything", "goes"], TEST_PROGRAM, panic_provider)
    assert [s.kind for s in context.stages] == ["program", "escape", "escaped", "escaped"]


def test_walker_external_subcommand_and_global_flags():
    command = (
        "nrfutil --log-level info device program "
        "--firmware app.hex --serial-number 1 --log-level off"
    )
    vnc.validate_command(command, NRFUTIL_PROGRAM, device_provider)


def test_validate_command_errors():
    with pytest.raises(vnc.ValidationError, match="unrecognized syntax element '--core'"):
        vnc.validate_command("nrfutil device program --core App", NRFUTIL_PROGRAM, device_provider)

    with pytest.raises(vnc.ValidationError, match="Could not install"):
        vnc.validate_command("nrfutil devic list", NRFUTIL_PROGRAM, device_provider)

    with pytest.raises(vnc.ValidationError, match="Failed to parse as shell command"):
        vnc.validate_command('nrfutil device list "unclosed', NRFUTIL_PROGRAM, device_provider)


def test_tokenize_strips_shell_syntax_and_joins_placeholders():
    assert vnc.tokenize("nrfutil device list --json > out.json") == [
        "nrfutil",
        "device",
        "list",
        "--json",
    ]
    assert vnc.tokenize("nrfutil device list | grep x") == ["nrfutil", "device", "list"]
    assert vnc.tokenize("nrfutil device program --serial-number <J-Link Serial number>") == [
        "nrfutil",
        "device",
        "program",
        "--serial-number",
        "<J-Link_Serial_number>",
    ]


def test_iter_commands_prompts_continuations_and_other_lines():
    lines = [
        "cd build",
        "$ nrfutil device list",
        "",
        "> nrfutil device program \\",
        "    --firmware app.hex",
        "west build",
        "nrfutil device reset \\",
    ]
    assert list(vnc.iter_commands(lines)) == [
        (1, "nrfutil device list"),
        (3, "nrfutil device program --firmware app.hex"),
        (6, "nrfutil device reset"),
    ]

"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import validate_nrfutil_commands as vnc  # noqa: E402

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
    with pytest.raises(vnc.ValidationError, match="unrecognised element '--core'"):
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


def extract(content):
    return [(e.line, e.command, e.skip) for e in vnc.extract_examples(Path("test.rst"), content)]


def test_extract_literal_block():
    content = """\
Run the following command::

   nrfutil device list

nrfutil is not a command in a paragraph.
"""
    assert extract(content) == [(3, "nrfutil device list", False)]


def test_extract_list_item_literal_block_ends_at_paragraph():
    content = """\
2. Read the memory::

      nrfutil device read --address 0x0

   nrfutil returns the value.
"""
    assert extract(content) == [(3, "nrfutil device read --address 0x0", False)]


def test_extract_code_block_with_options_and_prompts():
    content = """\
.. code-block:: console
   :class: highlight

   $ nrfutil device list
   > nrfutil device reset
   some output
   nrfutil device recover

Not part of the block.
"""
    assert extract(content) == [
        (4, "nrfutil device list", False),
        (5, "nrfutil device reset", False),
        (7, "nrfutil device recover", False),
    ]


def test_extract_parsed_literal():
    content = """\
.. parsed-literal::
   :class: highlight

   nrfutil install device=\\ |54H_nrfutil_device_ver|\\  --force
   nrfutil sdk-manager toolchain launch --ncs-version *version* --shell
"""
    assert extract(content) == [
        (4, "nrfutil install device=X --force", False),
        (5, "nrfutil sdk-manager toolchain launch --ncs-version version --shell", False),
    ]


def test_extract_nested_tabs_and_continuation(monkeypatch):
    monkeypatch.setattr(vnc.sys, "platform", "linux")
    content = """\
.. tabs::

   .. tab:: Linux

      .. code-block:: console

         nrfutil device program \\
           --firmware app.hex

   .. tab:: Windows

      Text::

         nrfutil device list
"""
    assert extract(content) == [
        (7, "nrfutil device program --firmware app.hex", False),
        (14, "nrfutil device list", True),
    ]


def test_extract_markers_apply_to_next_command_only():
    content = """\
.. validate command: skip next

.. code-block:: console

   nrfutil device x-read
   nrfutil device list

.. validate command: windows

.. code-block:: console

   nrfutil sdk-manager toolchain launch --terminal
"""
    examples = extract(content)
    assert examples[0] == (5, "nrfutil device x-read", True)
    assert examples[1] == (6, "nrfutil device list", False)
    assert examples[2] == (
        12,
        "nrfutil sdk-manager toolchain launch --terminal",
        sys.platform != "win32",
    )


def test_extract_platform_tabs(monkeypatch):
    monkeypatch.setattr(vnc.sys, "platform", "linux")
    content = """\
.. tabs::

   .. tab:: Windows

      .. parsed-literal::
         :class: highlight

         nrfutil sdk-manager toolchain launch --terminal

   .. tab:: Linux

      Run this::

         nrfutil sdk-manager toolchain launch --shell

   .. group-tab:: macOS

      .. validate command: linux

      .. code-block:: console

         nrfutil device list

.. code-block:: console

   nrfutil device erase
"""
    assert extract(content) == [
        (8, "nrfutil sdk-manager toolchain launch --terminal", True),
        (14, "nrfutil sdk-manager toolchain launch --shell", False),
        (22, "nrfutil device list", False),
        (26, "nrfutil device erase", False),
    ]


def test_extract_unknown_marker():
    with pytest.raises(vnc.ValidationError, match="Unknown validate command marker"):
        extract(".. validate command: sometimes\n")


def test_directive_without_literal_body_is_ignored():
    content = """\
.. note::

   nrfutil device list is mentioned in a note.
"""
    assert extract(content) == []


def test_load_pinned_versions(tmp_path, monkeypatch):
    monkeypatch.setattr(vnc.sys, "platform", "linux")
    scripts = tmp_path / "scripts"
    scripts.mkdir()
    (scripts / "tools-versions-linux.yml").write_text(
        "nrfutil:\n  version: 8.1.1\n  subcommands:\n    device: 2.20.0\n", encoding="utf-8"
    )
    shortcuts = tmp_path / "doc" / "nrf"
    shortcuts.mkdir(parents=True)
    (shortcuts / "shortcuts.txt").write_text(
        ".. |54H_nrfutil_device_ver| replace:: 2.17.2\n"
        ".. |54H_nrfutil_trace_ver| replace:: 4.4.0\n"
        ".. |release| replace:: v3.2.0\n",
        encoding="utf-8",
    )
    assert vnc.load_pinned_versions(tmp_path) == {"device": "2.20.0", "trace": "4.4.0"}
    assert vnc.load_core_version(tmp_path) == "8.1.1"


def test_find_rst_files_excludes(tmp_path):
    (tmp_path / "doc" / "keep").mkdir(parents=True)
    (tmp_path / "doc" / "skip").mkdir(parents=True)
    (tmp_path / "doc" / "keep" / "a.rst").write_text("", encoding="utf-8")
    (tmp_path / "doc" / "skip" / "b.rst").write_text("", encoding="utf-8")

    files = vnc.find_rst_files(tmp_path, [], ["doc/skip/**"])
    assert [f.relative_to(tmp_path).as_posix() for f in files] == ["doc/keep/a.rst"]


def test_report_error_annotates_in_github_actions(monkeypatch, capsys):
    monkeypatch.setenv("GITHUB_ACTIONS", "true")
    vnc.report_error("Unknown flag '--stdout'\nCommand: nrfutil trace", "doc/a,b.rst", 157)
    assert capsys.readouterr().out.splitlines() == [
        "ERROR: doc/a,b.rst:157: Unknown flag '--stdout'",
        "Command: nrfutil trace",
        "::error title=nRF Util command check,file=doc/a%2Cb.rst,line=157::"
        "Unknown flag '--stdout'%0ACommand: nrfutil trace",
    ]


def test_report_error_without_github_actions(monkeypatch, capsys):
    monkeypatch.delenv("GITHUB_ACTIONS", raising=False)
    vnc.report_error("Unable to set up nrfutil")
    assert capsys.readouterr().out == "ERROR: Unable to set up nrfutil\n"


def test_find_rst_files_includes_txt_snippets(tmp_path):
    (tmp_path / "doc" / "includes").mkdir(parents=True)
    (tmp_path / "doc" / "includes" / "snippet.txt").write_text("", encoding="utf-8")
    (tmp_path / "doc" / "links.txt").write_text("", encoding="utf-8")
    (tmp_path / "doc" / "page.rst").write_text("", encoding="utf-8")

    files = vnc.find_rst_files(tmp_path, [], [])
    assert [f.relative_to(tmp_path).as_posix() for f in files] == [
        "doc/includes/snippet.txt",
        "doc/page.rst",
    ]

    explicit = [tmp_path / "doc" / "includes" / "snippet.txt", tmp_path / "doc" / "links.txt"]
    files = vnc.find_rst_files(tmp_path, explicit, [])
    assert [f.relative_to(tmp_path).as_posix() for f in files] == ["doc/includes/snippet.txt"]

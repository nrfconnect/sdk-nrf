"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import json
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "_extensions"))

from nrfutil_command import generate as gen  # noqa: E402
from nrfutil_command import specs  # noqa: E402


@pytest.fixture
def nrf_base(tmp_path, monkeypatch):
    """An sdk-nrf root on Linux, with empty pin files."""
    (tmp_path / "scripts").mkdir()
    (tmp_path / "doc" / "nrf").mkdir(parents=True)
    monkeypatch.setattr(specs, "NRF_BASE", tmp_path)
    monkeypatch.setattr(specs, "PINS_FILE", tmp_path / "pins.yml")
    monkeypatch.setattr(specs, "host_platform", lambda: "linux")
    write_pins(tmp_path)
    return tmp_path


def write_pins(root, tools_versions="", shortcuts="", pins=""):
    (root / "scripts" / "tools-versions-linux.yml").write_text(tools_versions, encoding="utf-8")
    (root / "doc" / "nrf" / "shortcuts.txt").write_text(shortcuts, encoding="utf-8")
    (root / "pins.yml").write_text(pins, encoding="utf-8")


def test_load_pinned_versions_precedence(nrf_base):
    write_pins(
        nrf_base,
        tools_versions="nrfutil:\n  version: 8.1.1\n  subcommands:\n    device: 2.20.0\n",
        shortcuts=".. |54H_nrfutil_device_ver| replace:: 2.17.2\n"
        ".. |54H_nrfutil_trace_ver| replace:: 4.4.0\n"
        ".. |release| replace:: v3.2.0\n",
        pins="trace: 4.0.0\nmcu-manager: 0.12.2\n",
    )
    assert specs.load_pinned_versions() == {
        "device": "2.20.0",
        "trace": "4.4.0",
        "mcu-manager": "0.12.2",
    }
    assert specs.load_core_version() == "8.1.1"


def test_load_pinned_versions_without_subcommands(nrf_base):
    write_pins(nrf_base, tools_versions="nrfutil:\n  version: 8.1.1\n  subcommands:\n")
    assert specs.load_pinned_versions() == {}


def test_numeric_command_names_are_strings(nrf_base):
    write_pins(
        nrf_base,
        tools_versions="nrfutil:\n  subcommands:\n    93-series: 1.0.0\n    91: 1.1.0\n",
        pins="91: 1.0.0\n",
    )
    assert specs.load_pinned_versions() == {"93-series": "1.0.0", "91": "1.1.0"}


def test_missing_pin_file_is_an_error(nrf_base):
    (nrf_base / "doc" / "nrf" / "shortcuts.txt").unlink()
    with pytest.raises(FileNotFoundError, match="shortcuts.txt"):
        specs.load_pinned_versions()


def test_snapshot_round_trip_and_matches(tmp_path):
    snapshot = specs.Snapshot(
        "8.1.1",
        {"flags": [], "description": "help"},
        {"device": "2.20.0"},
        {"device": {"subcommands": [], "description": "help"}},
        platform="windows",
    )
    snapshot.write(tmp_path)
    loaded = specs.Snapshot.load(tmp_path)

    assert loaded.platform == "windows"
    assert loaded.core == {"flags": []}
    assert loaded.commands == {"device": {"subcommands": []}}
    assert loaded.matches("8.1.1", {"device": "2.20.0"}, "windows")
    assert loaded.matches(None, {"device": "2.20.0"}, "windows")
    assert not loaded.matches("8.1.1", {"device": "2.20.0"}, "linux")
    assert not loaded.matches("8.2.0", {"device": "2.20.0"}, "windows")
    assert not loaded.matches("8.1.1", {"device": "2.21.0"}, "windows")
    assert not loaded.matches("8.1.1", {"device": "2.20.0", "trace": "4.4.0"}, "windows")


def test_unknown_command_suggests_pinning():
    snapshot = specs.Snapshot("8.1.1", {}, {}, {})
    with pytest.raises(specs.ValidationError, match="pin its version in .*pins.yml"):
        snapshot.program(["trace"])


class FakeNrfutil:
    def __init__(self, fail_on=None):
        self.calls = []
        self.fail_on = fail_on

    def run(self, args):
        self.calls.append(args)
        if args[0] == self.fail_on:
            raise gen.SnapshotError(f"'nrfutil {' '.join(args)}' failed")
        if args[-1] == "--json-args":
            return json.dumps({"name": args[0] if len(args) > 1 else "nrfutil"})
        return ""


@pytest.fixture
def fake_nrfutil(monkeypatch):
    fake = FakeNrfutil()
    monkeypatch.setattr(gen.Nrfutil, "run", lambda _self, args: fake.run(args))
    return fake


def test_generate_installs_pins_and_writes_snapshot(tmp_path, fake_nrfutil):
    snapshot = gen.generate(tmp_path, "8.1.1", {"trace": "4.4.0", "device": "2.20.0"}, "linux")

    assert fake_nrfutil.calls == [
        ["self-upgrade", "--to-version", "8.1.1"],
        ["--json-args"],
        ["install", "device=2.20.0"],
        ["device", "--json-args"],
        ["install", "trace=4.4.0"],
        ["trace", "--json-args"],
    ]
    assert snapshot.versions == {"device": "2.20.0", "trace": "4.4.0"}
    loaded = specs.Snapshot.load(tmp_path)
    assert loaded.commands["trace"] == {"name": "trace"}
    assert loaded.matches("8.1.1", {"device": "2.20.0", "trace": "4.4.0"}, "linux")


def test_failed_generate_keeps_previous_snapshot(tmp_path, fake_nrfutil):
    specs.Snapshot("8.1.1", {}, {"device": "2.20.0"}, {"device": {}}).write(tmp_path)
    fake_nrfutil.fail_on = "install"

    with pytest.raises(gen.SnapshotError):
        gen.generate(tmp_path, "8.1.1", {"device": "2.21.0"}, "linux")

    assert specs.Snapshot.load(tmp_path).versions == {"device": "2.20.0"}


def test_nrfutil_home_lasts_for_each_with_block():
    nrfutil = gen.Nrfutil()
    for _ in range(2):
        with nrfutil:
            home = Path(nrfutil._home.name)
            assert home.is_dir()
        assert not home.exists()


def test_missing_nrfutil_is_a_snapshot_error(tmp_path):
    with pytest.raises(gen.SnapshotError, match="Unable to run"):
        gen.generate(tmp_path, None, {}, "linux", executable=str(tmp_path / "no-nrfutil"))

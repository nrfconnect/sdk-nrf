"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import io
import json
import re
import sys
from pathlib import Path

import pytest

pytest.importorskip("sphinx")

from sphinx.application import Sphinx  # noqa: E402
from sphinx.util.console import nocolor  # noqa: E402

EXTENSIONS_DIR = Path(__file__).resolve().parents[2] / "_extensions"
sys.path.insert(0, str(EXTENSIONS_DIR))

from nrfutil_command import directive, specs  # noqa: E402
from nrfutil_command.specs import Snapshot  # noqa: E402

# The documentation button, in the block after the code
DOCS_BUTTON_RE = re.compile(
    r'<div class="nrfutil-command"><div class="highlight-nrfutil-console notranslate">'
    r'<div class="highlight"><pre>.*?</pre></div>\s*</div>\s*'
    r'<a class="nrfutil-command-docs-button[^>]*>.*?</a></div>',
    re.DOTALL,
)


def flag(long=None, short=None, values=()):
    return {"name": long or short, "long": long, "short": short, "values": list(values)}


CORE = {
    "allow_external_subcommands": True,
    "flags": [flag(long="version", short="V")],
    "arguments": [],
    "subcommands": [
        {
            "name": "install",
            "flags": [],
            "arguments": [{"name": "commands", "values": [{"optional": {"type": "STRING"}}]}],
            "subcommands": [],
        }
    ],
}

DEVICE = {
    "allow_external_subcommands": False,
    "flags": [],
    "arguments": [],
    "subcommands": [
        {
            "name": "program",
            "flags": [
                flag(long="firmware", values=[{"type": "FILE"}]),
                flag(long="serial-number", values=[{"type": "STRING"}]),
                flag(long="core", values=[{"possible": ["Application", "Network"]}]),
            ],
            "arguments": [],
            "subcommands": [],
        },
        {"name": "list", "flags": [], "arguments": [], "subcommands": []},
    ],
}

URLS = {
    "base_url": "https://docs.example.com/nrfutil/",
    "commands": {
        "device": "nrfutil-device/nrfutil-device.html",
        "device program": "nrfutil-device/nrfutil-device-program.html",
        "install": "nrfutil-core/nrfutil-install.html",
    },
}


def build(tmp_path, index, files=None, snapshot=None, status=None, builder="html", **config):
    src = tmp_path / "src"
    src.mkdir()
    specs_dir = tmp_path / "specs"
    snapshot = snapshot or Snapshot("8.1.1", CORE, {"device": "2.20.0"}, {"device": DEVICE})
    snapshot.write(specs_dir)
    urls = tmp_path / "command-urls.json"
    urls.write_text(json.dumps(URLS), encoding="utf-8")

    (src / "conf.py").write_text(
        f"import sys\n"
        f"sys.path.insert(0, {str(EXTENSIONS_DIR)!r})\n"
        f"extensions = ['nrfutil_command']\n"
        f"nrfutil_command_check = True\n"
        f"nrfutil_command_regenerate = False\n"
        f"nrfutil_command_specs = {str(specs_dir)!r}\n"
        f"nrfutil_command_urls = {str(urls)!r}\n"
        f"release = '3.4.0'\n"
        f"rst_epilog = '.. |device_ver| replace:: 2.20.0\\n.. |core ver| replace:: 8.1.1\\n'\n"
        + "".join(f"{key} = {value!r}\n" for key, value in config.items()),
        encoding="utf-8",
    )
    (src / "index.rst").write_text(index, encoding="utf-8")
    for name, content in (files or {}).items():
        (src / name).write_text(content, encoding="utf-8")

    nocolor()
    warnings = io.StringIO()
    app = Sphinx(
        str(src),
        str(src),
        str(tmp_path / "out"),
        str(tmp_path / "doctrees"),
        builder,
        status=status,
        warning=warnings,
        freshenv=True,
    )
    app.build()
    suffix = "html" if builder == "html" else "txt"
    html = (tmp_path / "out" / f"index.{suffix}").read_text(encoding="utf-8")
    # Several Sphinx applications in one process report the node classes they register again
    own = [line for line in warnings.getvalue().splitlines() if "already registered" not in line]
    return html, "\n".join(own)


def test_valid_command_renders_with_link(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device program --firmware app.hex\n",
    )
    assert warnings == ""
    assert "highlight-nrfutil-console" in html
    assert (
        'href="https://docs.example.com/nrfutil/nrfutil-device/nrfutil-device-program.html"' in html
    )
    assert 'aria-label="Open the nRF Util command documentation."' in html
    assert 'data-tooltip="Open command docs"' in html
    assert 'target="_blank" rel="noopener"' in html
    assert DOCS_BUTTON_RE.search(html)
    assert "nrfutil_command.css" in html


def test_other_builders_render_the_command(tmp_path):
    text, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device program --firmware <file>\n",
        builder="text",
    )
    assert warnings == ""
    assert "nrfutil device program --firmware <file>" in text


@pytest.mark.parametrize("language", ["shell", "none"])
def test_other_languages_mark_placeholders(tmp_path, language):
    html, warnings = build(
        tmp_path,
        f"Title\n=====\n\n.. nrfutil-command:: {language}\n\n"
        "   nrfutil device program --firmware <file>\n",
    )
    assert warnings == ""
    assert f'class="highlight-nrfutil-{language} notranslate"' in html
    assert '<span class="ge ge-Placeholder">&lt;file&gt;</span>' in html


def test_check_is_disabled_by_default(tmp_path, monkeypatch):
    monkeypatch.setattr(directive, "generate", None)
    html, warnings = build(tmp_path, INVALID_INDEX, nrfutil_command_check=False)
    assert warnings == ""
    assert "nrfutil-command-docs-button" not in html


def test_invalid_command_warns_at_line(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device list\n   nrfutil device lst\n",
    )
    assert "index.rst:7:" in warnings
    assert "Found unrecognized syntax element 'lst'" in warnings
    assert "nRF Util command: nrfutil device lst" in warnings


def test_commands_in_other_platform_tabs_are_not_validated(tmp_path):
    pytest.importorskip("sphinx_tabs")
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n"
        ".. tabs::\n\n"
        "   .. tab:: Windows\n\n"
        "      .. nrfutil-command::\n\n"
        "         nrfutil device lst\n\n"
        "   .. group-tab:: macOS\n\n"
        "      - Item\n\n"
        "        .. nrfutil-command::\n\n"
        "           nrfutil device lst\n\n"
        "   .. tab:: Linux\n\n"
        "      .. nrfutil-command::\n\n"
        "         nrfutil device lst\n",
        extensions=["nrfutil_command", "sphinx_tabs.tabs"],
    )
    assert warnings.count("Found unrecognized syntax element 'lst'") == 1
    assert "index.rst:24:" in warnings


def test_os_option_selects_blocks(tmp_path):
    pytest.importorskip("sphinx_tabs")
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n"
        ".. nrfutil-command::\n   :os: windows\n\n   nrfutil device lst\n\n"
        ".. nrfutil-command::\n   :os: unix\n\n   nrfutil device lst\n\n"
        ".. tabs::\n\n"
        "   .. tab:: Linux\n\n"
        "      .. nrfutil-command::\n         :os: macos\n\n"
        "         nrfutil device lst\n",
        extensions=["nrfutil_command", "sphinx_tabs.tabs"],
    )
    assert warnings.count("Found unrecognized syntax element 'lst'") == 1
    assert "index.rst:12:" in warnings


def test_os_option_rejects_unknown_values(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :os: android\n\n   nrfutil device list\n",
    )
    assert 'invalid option value: (option: "os"; value: \'android\')' in warnings


def test_no_validate_suppresses_warnings(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :no-validate:\n\n   nrfutil device lst\n",
    )
    assert warnings == ""


def test_unknown_command_suggests_pinning(tmp_path):
    _, warnings = build(tmp_path, "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil trace lte\n")
    assert "Unknown nrfutil command 'trace'" in warnings
    assert "pins.yml" in warnings


def test_substitutions_are_expanded(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n"
        "   nrfutil install device=|device_ver|\n"
        "   nrfutil device program --firmware app-|release|.hex\n",
    )
    assert warnings == ""
    assert "device=2.20.0" in html
    assert "app-3.4.0.hex" in html
    assert "|device_ver|" not in html


def test_substitution_names_are_normalized_like_docutils(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n"
        "   nrfutil install device=|Device_ver|\n"
        "   nrfutil self-upgrade --to-version |core   ver|\n",
        nrfutil_command_check=False,
    )
    assert warnings == ""
    assert "device=2.20.0" in html
    assert "--to-version 8.1.1" in html


def test_unknown_substitution_warns(tmp_path):
    _, warnings = build(
        tmp_path, "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil install device=|nope|\n"
    )
    assert "Unknown substitution '|nope|'" in warnings


def test_placeholders_are_accepted_and_emphasized(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n"
        "   nrfutil device program --firmware <hex file> --core <core> --serial-number <SN>\n",
    )
    assert warnings == ""
    # The console lexer splits <hex file> into several tokens, but it's marked as one placeholder
    assert '<span class="ge ge-Placeholder">&lt;hex file&gt;</span>' in html
    assert '<span class="ge ge-Placeholder">&lt;core&gt;</span>' in html


def test_button_opens_the_first_command(tmp_path):
    html, _ = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n"
        "   nrfutil device program --firmware a.hex\n"
        "   nrfutil install device\n",
    )
    assert html.count('class="nrfutil-command-docs-button') == 1
    assert "nrfutil-device-program.html" in html
    assert "nrfutil-install.html" not in html


def test_commands_that_fail_validation_keep_their_link(tmp_path):
    html, _ = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :os: windows\n\n"
        "   nrfutil device program --terminal\n",
    )
    assert "nrfutil-device/nrfutil-device-program.html" in html


def test_links_can_be_disabled(tmp_path):
    html, _ = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device list\n",
        nrfutil_command_links=False,
    )
    assert "nrfutil-command-docs" not in html


def test_caption_and_other_lines(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :caption: Program the device\n\n"
        "   cd build\n   nrfutil device program \\\n     --firmware app.hex\n",
    )
    assert warnings == ""
    assert "Program the device" in html
    assert DOCS_BUTTON_RE.search(html)


def test_included_file_reports_its_own_line(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. include:: snippet.txt\n",
        files={"snippet.txt": "Text.\n\n.. nrfutil-command::\n\n   nrfutil device lst\n"},
    )
    assert "snippet.txt:5:" in warnings


INVALID_INDEX = "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device lst\n"


@pytest.fixture
def nrf_base(tmp_path, monkeypatch):
    """An sdk-nrf root that pins nrfutil core 8.1.1 and device 2.21.0 on every platform."""
    root = tmp_path / "ncs"
    (root / "scripts").mkdir(parents=True)
    for name in specs.TOOLS_VERSIONS_FILES.values():
        (root / "scripts" / name).write_text(
            "nrfutil:\n  version: 8.1.1\n  subcommands:\n    device: 2.21.0\n", encoding="utf-8"
        )
    (root / "doc" / "nrf").mkdir(parents=True)
    (root / "doc" / "nrf" / "shortcuts.txt").write_text("", encoding="utf-8")
    (root / "pins.yml").write_text("", encoding="utf-8")
    monkeypatch.setattr(specs, "NRF_BASE", root)
    monkeypatch.setattr(specs, "PINS_FILE", root / "pins.yml")
    return root


def test_snapshot_with_other_versions_is_regenerated(tmp_path, nrf_base, monkeypatch):
    generated = []

    def fake_generate(directory, core_version, pins, platform, executable):
        generated.append((core_version, pins, platform, executable))
        return Snapshot(core_version, CORE, pins, {"device": DEVICE}, platform).write(directory)

    monkeypatch.setattr(directive.shutil, "which", lambda name: name)
    monkeypatch.setattr(directive, "generate", fake_generate)
    _, warnings = build(tmp_path, INVALID_INDEX, nrfutil_command_regenerate=True)

    assert generated == [("8.1.1", {"device": "2.21.0"}, specs.host_platform(), "nrfutil")]
    assert "Found unrecognized syntax element 'lst'" in warnings


def test_snapshot_with_pinned_versions_is_reused(tmp_path, nrf_base, monkeypatch):
    def fail(*_args):
        raise AssertionError("The snapshot was regenerated")

    monkeypatch.setattr(directive, "generate", fail)
    snapshot = Snapshot("8.1.1", CORE, {"device": "2.21.0"}, {"device": DEVICE})
    snapshot.platform = specs.host_platform()
    _, warnings = build(tmp_path, INVALID_INDEX, snapshot=snapshot, nrfutil_command_regenerate=True)
    assert "Found unrecognized syntax element 'lst'" in warnings


def test_missing_nrfutil_warns(tmp_path, nrf_base):
    html, warnings = build(
        tmp_path,
        INVALID_INDEX,
        nrfutil_command_regenerate=True,
        nrfutil_command_nrfutil=str(tmp_path / "no-nrfutil"),
    )
    assert "nrfutil commands are not validated" in warnings
    assert "was not found on the PATH" in warnings
    assert "Found unrecognized syntax element" not in warnings
    assert "nrfutil device lst" in html


def test_failed_generation_warns(tmp_path, nrf_base, monkeypatch):
    def fail(*_args):
        raise directive.SnapshotError("'nrfutil install device' failed")

    monkeypatch.setattr(directive.shutil, "which", lambda name: name)
    monkeypatch.setattr(directive, "generate", fail)
    _, warnings = build(tmp_path, INVALID_INDEX, nrfutil_command_regenerate=True)
    assert "Failed to generate the nrfutil command snapshot" in warnings
    assert "Found unrecognized syntax element" not in warnings


def test_snapshot_platform_selects_tabs(tmp_path):
    pytest.importorskip("sphinx_tabs")
    snapshot = Snapshot("8.1.1", CORE, {"device": "2.20.0"}, {"device": DEVICE}, "windows")
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n"
        ".. tabs::\n\n"
        "   .. tab:: Windows\n\n"
        "      .. nrfutil-command::\n\n"
        "         nrfutil device lst\n\n"
        "   .. tab:: Linux\n\n"
        "      .. nrfutil-command::\n\n"
        "         nrfutil device lst\n",
        snapshot=snapshot,
        extensions=["nrfutil_command", "sphinx_tabs.tabs"],
    )
    assert warnings.count("Found unrecognized syntax element 'lst'") == 1
    assert "index.rst:10:" in warnings

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

# The documentation button, moved into the block after the command
DOCS_BUTTON_RE = re.compile(
    r'</pre><a class="nrfutil-command-docs-button[^>]*>.*?</a></div>\s*</div>'
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


def build(tmp_path, index, files=None, snapshot=None, status=None, **config):
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
        f"nrfutil_command_specs = {str(specs_dir)!r}\n"
        f"nrfutil_command_urls = {str(urls)!r}\n"
        f"release = '3.4.0'\n"
        f"rst_epilog = '.. |device_ver| replace:: 2.20.0\\n'\n"
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
        "html",
        status=status,
        warning=warnings,
        freshenv=True,
    )
    app.build()
    html = (tmp_path / "out" / "index.html").read_text(encoding="utf-8")
    # Several Sphinx applications in one process report the node classes they register again
    own = [line for line in warnings.getvalue().splitlines() if "already registered" not in line]
    return html, "\n".join(own)


def test_valid_command_renders_with_link(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n\n   nrfutil device program --firmware app.hex\n",
    )
    assert warnings == ""
    assert "nrfutil-command" in html
    assert "highlight-console" in html
    assert (
        'href="https://docs.example.com/nrfutil/nrfutil-device/nrfutil-device-program.html"' in html
    )
    assert 'aria-label="Open the nRF Util command documentation."' in html
    assert 'data-tooltip="Open command docs"' in html
    assert 'target="_blank" rel="noopener"' in html
    # The button is moved into the block, where the copy button goes
    assert DOCS_BUTTON_RE.search(html)
    # Without values, the block has no controls and no footer
    assert "nrfutil-command-footer" not in html
    assert "nrfutil_command.css" in html
    assert '<div class="highlight-console notranslate nrfutil-command">' in html


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
    assert '<em class="nrfutil-placeholder" data-name="hex file">&lt;hex file&gt;</em>' in html
    assert '<em class="nrfutil-placeholder" data-name="core">&lt;core&gt;</em>' in html


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
def ncs_root(tmp_path, monkeypatch):
    """An sdk-nrf root that pins nrfutil core 8.1.1 and device 2.21.0 on every platform."""
    root = tmp_path / "ncs"
    (root / "scripts").mkdir(parents=True)
    for name in specs.TOOLS_VERSIONS_FILES.values():
        (root / "scripts" / name).write_text(
            "nrfutil:\n  version: 8.1.1\n  subcommands:\n    device: 2.21.0\n", encoding="utf-8"
        )
    monkeypatch.setattr(specs, "PINS_FILE", tmp_path / "no-pins.yml")
    return root


def test_snapshot_with_other_versions_is_regenerated(tmp_path, ncs_root, monkeypatch):
    generated = []

    def fake_generate(directory, core_version, pins, platform, executable):
        generated.append((core_version, pins, platform, executable))
        return Snapshot(core_version, CORE, pins, {"device": DEVICE}, platform).write(directory)

    monkeypatch.setattr(directive.shutil, "which", lambda name: name)
    monkeypatch.setattr(directive, "generate", fake_generate)
    _, warnings = build(tmp_path, INVALID_INDEX, nrfutil_command_ncs_root=str(ncs_root))

    assert generated == [("8.1.1", {"device": "2.21.0"}, specs.host_platform(), "nrfutil")]
    assert "Found unrecognized syntax element 'lst'" in warnings


def test_snapshot_with_pinned_versions_is_reused(tmp_path, ncs_root, monkeypatch):
    def fail(*_args):
        raise AssertionError("The snapshot was regenerated")

    monkeypatch.setattr(directive, "generate", fail)
    snapshot = Snapshot("8.1.1", CORE, {"device": "2.21.0"}, {"device": DEVICE})
    snapshot.platform = specs.host_platform()
    _, warnings = build(
        tmp_path, INVALID_INDEX, snapshot=snapshot, nrfutil_command_ncs_root=str(ncs_root)
    )
    assert "Found unrecognized syntax element 'lst'" in warnings


def test_missing_nrfutil_skips_validation_with_a_message(tmp_path, ncs_root):
    status = io.StringIO()
    html, warnings = build(
        tmp_path,
        INVALID_INDEX,
        status=status,
        nrfutil_command_ncs_root=str(ncs_root),
        nrfutil_command_nrfutil=str(tmp_path / "no-nrfutil"),
    )
    assert warnings == ""
    assert "nrfutil device lst" in html
    assert "Skipping the validation of nrfutil commands" in status.getvalue()
    assert "was not found on the PATH" in status.getvalue()


def test_missing_nrfutil_warns_when_required(tmp_path, ncs_root):
    _, warnings = build(
        tmp_path,
        INVALID_INDEX,
        nrfutil_command_ncs_root=str(ncs_root),
        nrfutil_command_nrfutil=str(tmp_path / "no-nrfutil"),
        nrfutil_command_required=True,
    )
    assert "was not found on the PATH" in warnings
    assert "Found unrecognized syntax element" not in warnings


def test_failed_generation_skips_validation(tmp_path, ncs_root, monkeypatch):
    def fail(*_args):
        raise directive.SnapshotError("'nrfutil install device' failed")

    monkeypatch.setattr(directive.shutil, "which", lambda name: name)
    monkeypatch.setattr(directive, "generate", fail)
    status = io.StringIO()
    _, warnings = build(
        tmp_path, INVALID_INDEX, status=status, nrfutil_command_ncs_root=str(ncs_root)
    )
    assert warnings == ""
    assert "Failed to generate the nrfutil command snapshot" in status.getvalue()


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


VALUES_INDEX = (
    "Title\n=====\n\n.. nrfutil-command::\n"
    "   :values: core=Network\n"
    "   :values-windows: image=app.hex\n"
    "   :values-linux: image=app.elf\n\n"
    "   nrfutil device program --firmware <image> --core <core> --serial-number <SN>\n"
)


def test_values_render_controls_and_placeholders(tmp_path):
    html, warnings = build(tmp_path, VALUES_INDEX)
    assert warnings == ""
    controls = re.search(r'<div class="nrfutil-command-controls"([^>]*)></div>', html)
    assert controls
    assert "data-values=\"{&quot;core&quot;: &quot;Network&quot;}\"" in controls.group(1)
    assert "&quot;windows&quot;: {&quot;image&quot;: &quot;app.hex&quot;}" in controls.group(1)
    # The documentation button stays next to the copy button, and the controls go in the footer
    # inside the border of the block
    assert re.search(
        r'</pre><a class="nrfutil-command-docs-button[^>]*>.*?</a></div>'
        r'<div class="nrfutil-command-footer"><div class="nrfutil-command-controls"[^>]*></div>'
        r"</div>\s*</div>",
        html,
    )
    assert '<em class="nrfutil-placeholder" data-name="core">&lt;core&gt;</em>' in html
    # Without JavaScript, operating system placeholders show the values of the first system
    assert '<em class="nrfutil-placeholder nrfutil-os-value" data-name="image">app.hex</em>' in html
    assert "nrfutil_command.js" in html
    # The example values switch starts on
    assert "data-example-toggle-off" not in html


def test_example_toggle_off_starts_with_the_placeholders(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n"
        "   :values: core=Network\n"
        "   :example-toggle-off:\n\n"
        "   nrfutil device program --firmware app.hex --core <core>\n",
    )
    assert warnings == ""
    assert re.search(
        r'<div class="nrfutil-command-controls"[^>]* data-example-toggle-off></div>', html
    )
    # The rewrite still finds the controls, so the placeholders are marked
    assert 'data-name="core"' in html


def test_example_toggle_off_without_values_warns(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :example-toggle-off:\n\n   nrfutil device list\n",
    )
    assert "The :example-toggle-off: option has no effect without :values:" in warnings


def test_values_expand_substitutions(tmp_path):
    html, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :values: file=app-|release|.hex\n\n"
        "   nrfutil device program --firmware <file>\n",
    )
    assert warnings == ""
    assert "app-3.4.0.hex" in html


def test_example_values_are_validated(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :values: subcommand=lst\n\n"
        "   nrfutil device <subcommand>\n",
    )
    assert "With the example values: Found unrecognized syntax element 'lst'" in warnings
    assert "nRF Util command: nrfutil device lst" in warnings


def test_os_values_are_validated_for_the_snapshot_platform(tmp_path):
    snapshot = Snapshot("8.1.1", CORE, {"device": "2.20.0"}, {"device": DEVICE}, "windows")
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n"
        "   :values-windows: list=list\n"
        "   :values-linux: list=lst\n\n"
        "   nrfutil device <list>\n",
        snapshot=snapshot,
    )
    assert warnings == ""

    snapshot = Snapshot("8.1.1", CORE, {"device": "2.20.0"}, {"device": DEVICE}, "linux")
    linux_dir = tmp_path / "linux"
    linux_dir.mkdir()
    _, warnings = build(
        linux_dir,
        "Title\n=====\n\n.. nrfutil-command::\n"
        "   :values-windows: list=list\n"
        "   :values-linux: list=lst\n\n"
        "   nrfutil device <list>\n",
        snapshot=snapshot,
    )
    assert warnings.count("Found unrecognized syntax element 'lst'") == 1


def test_values_without_placeholder_or_system_warn(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n"
        "   :values: nope=1\n"
        "   :values-windows: image=app.hex\n"
        "   :values-linux: other=x\n\n"
        "   nrfutil device program --firmware <image>\n",
    )
    assert "Value for '<nope>', which isn't a placeholder" in warnings
    assert "Value for '<other>', which isn't a placeholder" in warnings
    assert "Placeholder '<image>' has no value in :values-linux:" in warnings
    assert "Placeholder '<other>' has no value in :values-windows:" in warnings


def test_values_must_be_name_value_pairs(tmp_path):
    _, warnings = build(
        tmp_path,
        "Title\n=====\n\n.. nrfutil-command::\n   :values: version\n\n   nrfutil device list\n",
    )
    assert "expected name=value, got 'version'" in warnings

# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Pytest configuration, hooks, and fixtures for MCUboot test suite."""

import os
import sys
import textwrap

import pytest

# Add the directory to PYTHONPATH
zephyr_base = os.getenv("ZEPHYR_BASE")
if zephyr_base:
    sys.path.insert(0, os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src"))
else:
    raise EnvironmentError("ZEPHYR_BASE environment variable is not set")

pytest_plugins = [
    "twister_harness.plugin",
]

USED_MARKERS = [
    # Test cycle:
    "commit: run on commit, every test without nightly or weekly marker get this marker",
    "nightly: use to skip tests in the on commit regression, tip: use -m 'nightly or commit' in regression",
    "weekly: for weekly run, tip: use -m 'weekly or nightly or commit' in regression to run all tests",
    textwrap.dedent("""
        add_markers_if(condition, markers): decorate test with given markers
             if the condition evaluate to True.
        Example: add_markers_if('"nsib" in device_config.build_dir.name', [pytest.mark.nightly])
    """),
    # filtering:
    textwrap.dedent("""
        skip_if(condition, reason=...): skip the given test function if the condition evaluate to True.
            'DeviceConfig' object is available as 'device_config' variable in the condition.
        Example: skip_if('"nrf54l" in device_config.platform', reason='Filtered out for nrf54l family')
    """),
]


def pytest_configure(config: pytest.Config):
    """Configure pytest markers for the test session."""
    for marker in USED_MARKERS:
        config.addinivalue_line("markers", marker)


def pytest_collection_modifyitems(config: pytest.Config, items: list[pytest.Item]):  # noqa: ARG001
    """Modify collected test items before running tests."""
    for item in items:
        # Implementation for add_markers_if decorator
        for marker in item.iter_markers("add_markers_if"):
            try:
                condition = marker.args[0]
                markers = marker.args[1]
                if not isinstance(markers, list):
                    markers = [markers]
            except (IndexError, KeyError):
                pytest.exit("Wrong usage of `add_markers_if` decorator")
            if _evaluate_condition(item, condition):
                for marker in markers:
                    item.add_marker(marker)

        # Add default markers if not any of the used markers are present
        if not any(marker.name in ["commit", "nightly", "weekly"] for marker in item.iter_markers()):
            item.add_marker(pytest.mark.commit)
            item.add_marker(pytest.mark.nightly)
            item.add_marker(pytest.mark.weekly)

        # Implementation for skip_if decorator
        for marker in item.iter_markers("skip_if"):
            try:
                condition = marker.args[0]
                reason = marker.kwargs.get("reason", "")
            except (IndexError, KeyError):
                pytest.exit("Wrong usage of `skip_if` decorator")
            if _evaluate_condition(item, condition):
                item.add_marker(pytest.mark.skip(reason))


def _evaluate_condition(item, condition: str) -> bool:
    assert isinstance(condition, str)
    if not condition:
        return True
    globals_ = {"__builtins__": {"sys": sys, "os": os, "any": any, "all": all}}
    try:
        device_config = item.config.twister_harness_config.devices[0]
        locals_ = {
            "device_config": device_config,
        }
    except (KeyError, AttributeError):
        return True
    try:
        return eval(condition, globals_, locals_) is True
    except Exception as exc:
        pytest.exit(f'Cannot evaluate expression: "{condition}"; {exc}')

# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""System tests for tampered application security scenarios."""

import shutil
from pathlib import Path

import pytest
from fw_info_hex_utils import (
    FW_INFO_MAGIC_LEN,
    FW_INFO_UNSUPPORTED_OFFSET,
    _app_config_path,
    assert_fw_info_discovery_rejection,
    get_firmware_info_magic_bytes,
    get_fw_info_search_base,
    locate_fw_info_address,
)
from intelhex import IntelHex
from twister_harness import DeviceAdapter
from twister_harness_ext.utils.dts_helper import get_partition_address


def _signed_b0_hex(dut: DeviceAdapter) -> Path:
    assert dut.device_config.app_build_dir is not None
    app_name = dut.device_config.app_build_dir.name
    return dut.device_config.build_dir / f"signed_by_b0_{app_name}.hex"


def _signed_b0_s1_hex(dut: DeviceAdapter) -> Path:
    assert dut.device_config.app_build_dir is not None
    app_name = dut.device_config.app_build_dir.name
    return dut.device_config.build_dir / f"signed_by_b0_{app_name}_s1_variant.hex"


def _pristine_hex_path(hex_path: Path) -> Path:
    return hex_path.with_name(hex_path.stem + ".pristine" + hex_path.suffix)


def _ensure_pristine_hex_backups(dut: DeviceAdapter) -> None:
    for hex_path in (_signed_b0_hex(dut), _signed_b0_s1_hex(dut)):
        if not hex_path.is_file():
            continue
        pristine = _pristine_hex_path(hex_path)
        if not pristine.is_file():
            shutil.copy2(hex_path, pristine)


def _restore_signed_hex_from_pristine(dut: DeviceAdapter) -> None:
    for hex_path in (_signed_b0_hex(dut), _signed_b0_s1_hex(dut)):
        pristine = _pristine_hex_path(hex_path)
        if pristine.is_file():
            shutil.copy2(pristine, hex_path)


@pytest.fixture(autouse=True)
def _reset_signed_hex_before_test(unlaunched_dut: DeviceAdapter):
    """Each pytest case starts from unmodified signed images (tests share one build dir)."""
    _ensure_pristine_hex_backups(unlaunched_dut)
    _restore_signed_hex_from_pristine(unlaunched_dut)


def _edt_pickle(dut: DeviceAdapter) -> Path:
    assert dut.device_config.app_build_dir is not None
    return dut.device_config.app_build_dir / "zephyr" / "edt.pickle"


def _load_signed_b0_intelhex(dut: DeviceAdapter) -> IntelHex:
    app_hex = IntelHex(str(_signed_b0_hex(dut)))
    s1_hex_path = _signed_b0_s1_hex(dut)
    if s1_hex_path.is_file():
        app_hex.merge(IntelHex(str(s1_hex_path)))
    return app_hex


def _write_signed_b0_images(dut: DeviceAdapter, app_hex: IntelHex) -> None:
    edt = _edt_pickle(dut)
    s0_base = get_partition_address(edt, "s0_partition", absolute=True)
    s1_base = get_partition_address(edt, "s1_partition", absolute=True)

    main_out = IntelHex(str(_pristine_hex_path(_signed_b0_hex(dut))))
    s1_path = _signed_b0_s1_hex(dut)
    s1_out = IntelHex(str(_pristine_hex_path(s1_path))) if s1_path.is_file() else None

    for addr in app_hex.addresses():
        value = app_hex[addr]
        if addr >= s1_base and s1_out is not None:
            s1_out[addr] = value
        elif addr >= s0_base:
            main_out[addr] = value

    main_out.write_hex_file(str(_signed_b0_hex(dut)))
    if s1_out is not None:
        s1_out.write_hex_file(str(s1_path))


def _write_and_launch(unlaunched_dut: DeviceAdapter, app_hex: IntelHex) -> list[str]:
    _write_signed_b0_images(unlaunched_dut, app_hex)
    unlaunched_dut.launch()
    return unlaunched_dut.readlines_until(regex="Hello World!", timeout=30)


def test_tamper_s0_boot_s1(unlaunched_dut: DeviceAdapter):
    """Verify that the application in slot 1 boots when slot 0 is tampered.

    APP based on hello_world, the application in slot 0 is tampered before flashing the DUT.
    Image in slot 1 boots correctly. Image in slot 0 is permanently invalidated.
    """
    app_hex = _load_signed_b0_intelhex(unlaunched_dut)
    s0_address: int = get_partition_address(
        _edt_pickle(unlaunched_dut),
        "s0_partition",
        absolute=True,
    )
    app_hex.puts(s0_address, b"\xff")
    lines = _write_and_launch(unlaunched_dut, app_hex)
    pytest.LineMatcher(lines).fnmatch_lines(
        [
            "*Attempting to boot slot 0*",
            "*Failed to validate, permanently invalidating!*",
            "*Attempting to boot slot 1*",
            "*I: Firmware signature verified*",
        ]
    )


@pytest.mark.nightly
def test_s0_no_discoverable_fw_info_boots_s1(unlaunched_dut: DeviceAdapter):
    """Slot 0 is skipped when fw_info magic is missing; slot 1 boots without validating s0."""
    magic = get_firmware_info_magic_bytes(_app_config_path(unlaunched_dut))
    search_base = get_fw_info_search_base(
        unlaunched_dut, _edt_pickle(unlaunched_dut), "s0_partition"
    )
    app_hex = _load_signed_b0_intelhex(unlaunched_dut)
    magic_addr = locate_fw_info_address(app_hex, search_base, magic)
    app_hex.puts(magic_addr, b"\xff" * FW_INFO_MAGIC_LEN)

    lines = _write_and_launch(unlaunched_dut, app_hex)
    matcher = pytest.LineMatcher(lines)
    matcher.fnmatch_lines(
        [
            "*Attempting to boot slot 1*",
            "*I: Firmware signature verified*",
        ]
    )
    matcher.no_fnmatch_line("*Attempting to boot slot 0*")
    assert_fw_info_discovery_rejection(matcher)


@pytest.mark.nightly
def test_both_slots_no_discoverable_fw_info(unlaunched_dut: DeviceAdapter):
    """When no slot exposes discoverable fw_info, NSIB does not boot an application."""
    magic = get_firmware_info_magic_bytes(_app_config_path(unlaunched_dut))
    edt = _edt_pickle(unlaunched_dut)
    app_hex = _load_signed_b0_intelhex(unlaunched_dut)

    for partition in ("s0_partition", "s1_partition"):
        search_base = get_fw_info_search_base(unlaunched_dut, edt, partition)
        magic_addr = locate_fw_info_address(app_hex, search_base, magic)
        app_hex.puts(magic_addr, b"\xff" * FW_INFO_MAGIC_LEN)

    _write_signed_b0_images(unlaunched_dut, app_hex)
    unlaunched_dut.launch()

    lines = unlaunched_dut.readlines_until(regex="No bootable image found", timeout=30)
    matcher = pytest.LineMatcher(lines)
    matcher.fnmatch_lines(["*No bootable image found*"])
    matcher.no_fnmatch_line("*Attempting to boot slot 0*")
    matcher.no_fnmatch_line("*Attempting to boot slot 1*")
    matcher.no_fnmatch_line("*Hello World*")


@pytest.mark.nightly
def test_decoy_fw_info_at_unsupported_offset_ignored(unlaunched_dut: DeviceAdapter):
    """Valid magic at a non-searchable offset does not satisfy fw_info discovery."""
    magic = get_firmware_info_magic_bytes(_app_config_path(unlaunched_dut))
    edt = _edt_pickle(unlaunched_dut)
    app_hex = _load_signed_b0_intelhex(unlaunched_dut)

    for partition in ("s0_partition", "s1_partition"):
        search_base = get_fw_info_search_base(unlaunched_dut, edt, partition)
        magic_addr = locate_fw_info_address(app_hex, search_base, magic)
        app_hex.puts(magic_addr, b"\xff" * FW_INFO_MAGIC_LEN)
        app_hex.puts(search_base + FW_INFO_UNSUPPORTED_OFFSET, magic)

    _write_signed_b0_images(unlaunched_dut, app_hex)
    unlaunched_dut.launch()

    lines = unlaunched_dut.readlines_until(regex="No bootable image found", timeout=30)
    matcher = pytest.LineMatcher(lines)
    matcher.fnmatch_lines(["*No bootable image found*"])
    matcher.no_fnmatch_line("*Attempting to boot slot 0*")
    matcher.no_fnmatch_line("*Attempting to boot slot 1*")
    matcher.no_fnmatch_line("*Hello World*")

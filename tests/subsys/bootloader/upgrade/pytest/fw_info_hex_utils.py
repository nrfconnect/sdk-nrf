# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Helpers for NSIB firmware information (fw_info) discovery tests in Intel HEX images."""

from __future__ import annotations

import struct
from pathlib import Path

from intelhex import IntelHex
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config
from twister_harness_ext.utils.dts_helper import get_partition_address

FW_INFO_ALLOWED_OFFSETS: tuple[int, ...] = (
    0x0,
    0x200,
    0x400,
    0x600,
    0x800,
    0xE00,
    0x1000,
)

FW_INFO_MAGIC_LEN = 12
FW_INFO_UNSUPPORTED_OFFSET = 0x100


def _app_config_path(dut: DeviceAdapter) -> Path:
    assert dut.device_config.app_build_dir is not None
    return dut.device_config.app_build_dir / "zephyr" / ".config"


def _b0_config_path(dut: DeviceAdapter) -> Path:
    return dut.device_config.build_dir / "b0" / "zephyr" / ".config"


def _config_int(config_path: Path, key: str, default: int | None = None) -> int:
    raw = find_in_config(config_path, key)
    if raw is None or raw == "":
        if default is not None:
            return default
        raise ValueError(f"{key} not found in {config_path}")
    return int(raw, 0)


def get_firmware_info_magic_bytes(app_config: Path) -> bytes:
    """Build the 12-byte fw_info magic from Kconfig (same layout as FIRMWARE_INFO_MAGIC)."""
    common = _config_int(app_config, "CONFIG_FW_INFO_MAGIC_COMMON")
    firmware_info = _config_int(app_config, "CONFIG_FW_INFO_MAGIC_FIRMWARE_INFO")
    version = _config_int(app_config, "CONFIG_FW_INFO_VERSION")
    hardware_id = _config_int(app_config, "CONFIG_FW_INFO_HARDWARE_ID")
    crypto_id = _config_int(app_config, "CONFIG_FW_INFO_CRYPTO_ID")
    compatibility_id = _config_int(app_config, "CONFIG_FW_INFO_MAGIC_COMPATIBILITY_ID")
    magic_compat = version | (hardware_id << 8) | (crypto_id << 16) | (compatibility_id << 24)
    return struct.pack("<III", common, firmware_info, magic_compat)


def _b0_fw_info_find_base_offset(dut: DeviceAdapter) -> int:
    """Match samples/bootloader APP_HEADER_SKIP used before fw_info_find()."""
    b0_config = _b0_config_path(dut)
    mcuboot_image_number = _config_int(b0_config, "CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER", default=-1)
    if mcuboot_image_number == -1:
        return 0
    return _config_int(b0_config, "CONFIG_SB_IMAGE_BOOT_OFFSET", default=0)


def get_fw_info_search_base(
    dut: DeviceAdapter,
    edt_pickle: Path,
    partition: str = "s0_partition",
) -> int:
    """Address passed to fw_info_find(): partition base + B0 image boot skip."""
    slot_base = get_partition_address(edt_pickle, partition, absolute=True)
    return slot_base + _b0_fw_info_find_base_offset(dut)


def locate_fw_info_address(
    ih: IntelHex,
    search_base: int,
    magic: bytes,
) -> int:
    """Return the flash address of a discoverable fw_info magic header."""
    for offset in FW_INFO_ALLOWED_OFFSETS:
        addr = search_base + offset
        chunk = ih.gets(addr, FW_INFO_MAGIC_LEN)
        if chunk is not None and chunk == magic:
            return addr
    raise AssertionError(
        f"fw_info magic not found under search base 0x{search_base:x} "
        f"(offsets {[hex(o) for o in FW_INFO_ALLOWED_OFFSETS]})"
    )


def corrupt_fw_info_magic_patch(magic_addr: int) -> IntelHex:
    """Build a minimal HEX patch that corrupts fw_info magic in place."""
    patch = IntelHex()
    patch.puts(magic_addr, b"\xff" * FW_INFO_MAGIC_LEN)
    return patch


def assert_fw_info_discovery_rejection(matcher) -> None:
    """Serial lines when B0 skips a slot because fw_info is not discoverable."""
    matcher.no_fnmatch_line("*Attempting to boot slot 0*")
    matcher.no_fnmatch_line("*Failed to validate signature*")
    matcher.no_fnmatch_line("*Failed to validate, permanently invalidating!*")


def edt_pickle_for_build(dut: DeviceAdapter, build_dir: Path) -> Path:
    """EDT pickle for the application domain in a sysbuild output directory."""
    assert dut.device_config.app_build_dir is not None
    return build_dir / dut.device_config.app_build_dir.name / "zephyr" / "edt.pickle"

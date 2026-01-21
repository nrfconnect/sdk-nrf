"""Utilities for provisioning keys for KMU, NSIB, and MCUboot in test scenarios."""

from __future__ import annotations

import logging
import os
from pathlib import Path

from helpers import normalize_path, run_command
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config

ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]
APP_KEYS_FOR_KMU = Path(f"{ZEPHYR_BASE}/../nrf/tests/subsys/kmu/keys").resolve()

logger = logging.getLogger(__name__)


def provision_keys_for_kmu(
    keys: list[str] | str,
    keyname: str = "UROT_PUBKEY",  # UROT_PUBKEY, BL_PUBKEY, APP_PUBKEY
    policy: str | None = None,  # revokable, lock, lock-last (default)
    dev_id: str | None = None,
):
    """Provision keys for KMU using west ncs-provision upload command.

    Supports specifying keyname, SoC, policy, and device ID.
    """
    logger.info("Provision keys using west command.")
    command = ["west", "ncs-provision", "upload", "--keyname", keyname]
    if policy:
        command += ["--policy", policy]
    if dev_id:
        command += ["--dev-id", dev_id]
    if not isinstance(keys, list):
        keys = [keys]
    for key in keys:
        command += ["--key", normalize_path(str(key))]

    run_command(command)
    logger.info("Keys provisioned successfully")


def provision_nsib(dut: DeviceAdapter) -> None:
    """Provision NSIB keys using west ncs-provision upload command."""
    sysbuild_config = Path(dut.device_config.build_dir) / "zephyr" / ".config"
    if not find_in_config(sysbuild_config, "SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ED25519"):
        return

    logger.info("Provision KMU keys for NSIB")
    key_file = find_in_config(sysbuild_config, "SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE") or str(
        dut.device_config.build_dir / "GENERATED_NON_SECURE_SIGN_KEY_PRIVATE.pem"
    )
    provision_keys_for_kmu(key_file, keyname="BL_PUBKEY", dev_id=dut.device_config.id)


def get_keyname_for_mcuboot(sysbuild_config: Path) -> str:
    """Get the keyname for MCUboot based on the build configuration."""
    keyname = "BL_PUBKEY"
    if any(
        [
            find_in_config(sysbuild_config, "SB_CONFIG_SECURE_BOOT_APPCORE"),
            find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_SIGNATURE_KMU_UROT_MAPPING"),
        ]
    ):
        keyname = "UROT_PUBKEY"
    return keyname


def provision_mcuboot(dut: DeviceAdapter) -> None:
    """Provision MCUboot keys using west ncs-provision upload command."""
    sysbuild_config = Path(dut.device_config.build_dir) / "zephyr" / ".config"
    if not find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU"):
        return

    logger.info("Provision KMU keys for MCUboot")
    key_file = find_in_config(sysbuild_config, "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE")
    keyname = get_keyname_for_mcuboot(sysbuild_config)
    provision_keys_for_kmu(key_file, keyname, dev_id=dut.device_config.id)

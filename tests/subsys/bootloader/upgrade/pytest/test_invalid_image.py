# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for invalid signature and encryption scenarios in MCUboot application upgrades."""

from __future__ import annotations

import logging
from pathlib import Path

import pytest
from imgtool_wrapper import imgtool_keygen, imgtool_sign
from mcuboot_image_utils import copy_tlvs_areas
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def get_signature_type(sysbuild_config: Path) -> str:
    """Get and check the signature type from sysbuild config."""
    sig_type = find_in_config(sysbuild_config, "SB_CONFIG_SIGNATURE_TYPE")
    assert sig_type, "SB_CONFIG_SIGNATURE_TYPE not set in .config"
    return sig_type


def skip_ecdsa_p256_with_compression(sysbuild_config: Path) -> None:
    """Skip test for ECDSA_P256 with compression enabled."""
    if find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT"):
        # skip tests for nrf52840, because generated signature for edcsa-p256 can have extra
        # one byte, so we cannot copy TLVs
        pytest.skip("ECDSA_P256 signature has random size, image header will not be the same")


@pytest.mark.nightly
@pytest.mark.parametrize("test_option", ["no_key", "wrong_key", "wrong_key_good_tlv"])
def test_invalid_signature(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, test_option):
    """Test invalid signature scenarios for MCUboot application upgrade."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()

    imgtool_params = tm.build_params.imgtool_params
    app_to_sign = tm.build_params.app_to_sign

    if "no_key" in test_option:
        imgtool_params.key_file = None
    else:
        sysbuild_config = tm.build_params.build_dir / "zephyr" / ".config"
        sig_type = get_signature_type(sysbuild_config)
        if sig_type == "ECDSA_P256" and "good_tlv" in test_option:
            skip_ecdsa_p256_with_compression(sysbuild_config)

        # generate key file
        imgtool_params.key_file = app_to_sign.parent / f"generated_{sig_type}_{test_option}.pem"
        keyfile = imgtool_keygen(
            key_file=imgtool_params.key_file, key_type=sig_type, imgtool=imgtool_params.tool_path
        )
        assert keyfile.is_file(), f"Key file not found: {keyfile}"

    invalid_app = app_to_sign.parent / f"{app_to_sign.stem}_{test_option}.bin"
    imgtool_sign(app_to_sign=app_to_sign, imgtool_params=imgtool_params, output_bin=invalid_app)
    assert invalid_app.is_file()

    if "good_tlv" in test_option:
        # copy TLV from good image to invalid image
        invalid_app = copy_tlvs_areas(app_to_sign.parent / "zephyr.signed.bin", invalid_app)
        # because SHA is copied from actual image, mcumgr CLI tool receives error when trying
        # to test/confirm an image (same signature)
        tm.upload_images(invalid_app)
        tm.request_upgrade_from_shell()
        tm.reset_device_from_shell()
    else:
        tm.run_upgrade(invalid_app, confirm=True)

    tm.verify_after_reset(
        lines=[
            "Image in the secondary slot is not valid",
        ]
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info("Invalid image not upgraded")


@pytest.mark.nightly
@pytest.mark.parametrize("test_option", ["wrong_enc_key", "wrong_enc_key_good_tlv"])
def test_invalid_encryption(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, test_option):
    """Test invalid encryption scenarios for MCUboot application upgrade."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()

    imgtool_params = tm.build_params.imgtool_params
    app_to_sign = tm.build_params.app_to_sign

    sysbuild_config = tm.build_params.build_dir / "zephyr" / ".config"
    sig_type = get_signature_type(sysbuild_config)
    if sig_type == "ECDSA_P256" and "good_tlv" in test_option:
        skip_ecdsa_p256_with_compression(sysbuild_config)

    key_type = "x25519" if sig_type == "ED25519" else sig_type

    # generate key file
    imgtool_params.encryption_key_file = (
        app_to_sign.parent / f"generated_{key_type}_{test_option}.pem"
    )
    keyfile = imgtool_keygen(
        key_file=imgtool_params.encryption_key_file,
        key_type=key_type,
        imgtool=imgtool_params.tool_path,
    )
    assert keyfile.is_file(), f"Key file not found: {keyfile}"

    # generate image encrypted with wrong key
    invalid_app = app_to_sign.parent / f"{app_to_sign.stem}_{test_option}.bin"
    imgtool_sign(app_to_sign=app_to_sign, imgtool_params=imgtool_params, output_bin=invalid_app)
    assert invalid_app.is_file()

    if "good_tlv" in test_option:
        # copy TLV from good image to invalid image
        invalid_app = copy_tlvs_areas(
            app_to_sign.parent / "zephyr.signed.encrypted.bin", invalid_app
        )
        # because SHA is copied from actual image, mcumgr CLI tool receives error when trying
        # to test/confirm an image (same signature)
        tm.upload_images(invalid_app)
        tm.request_upgrade_from_shell()
        tm.reset_device_from_shell()
    else:
        tm.run_upgrade(invalid_app, confirm=True)

    tm.verify_after_reset(
        lines=[
            "Image in the secondary slot is not valid",
        ]
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info("Invalid image not upgraded")

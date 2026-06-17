# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Helpers for extracting and managing build parameters for MCUboot test scenarios."""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path

from twister_harness.helpers.utils import find_in_config
from twister_harness_ext.utils.dts_helper import (
    get_code_partition_address,
    get_code_slot,
    get_edt_node,
)
from twister_harness_ext.utils.imgtool_wrapper import ImgtoolParams

logger = logging.getLogger(__name__)


@dataclass
class BuildParameters:
    """Class for extracting and managing build parameters from build directories."""

    build_dir: Path
    app_build_dir: Path
    zephyr_base: Path
    zephyr_config: Path
    pm_config: Path
    pm: bool
    sysbuild: bool
    tfm: bool
    imgtool_params: ImgtoolParams
    net_core_name: str = "empty_net_core"
    mcuboot_dir: Path = field(init=False)
    app_to_sign: Path = field(init=False)
    netcore_to_sign: Path = field(init=False)
    mcuboot_secondary_app_to_sign: Path = field(init=False)

    @classmethod
    def create_from_ncs_build_dir(
        cls, build_dir: Path, app_build_dir: Path | None = None
    ) -> BuildParameters:
        """Create BuildParameters from NCS build directory and optional app build directory."""
        app_build_dir = app_build_dir or build_dir
        sysbuild = app_build_dir != build_dir
        zephyr_base = find_in_config(app_build_dir / "CMakeCache.txt", "ZEPHYR_BASE:PATH")
        zephyr_config = app_build_dir / "zephyr" / ".config"
        pm_config = build_dir / "pm.config"
        edt_data = app_build_dir / "zephyr" / "edt.pickle"
        tfm = find_in_config(zephyr_config, "CONFIG_BUILD_WITH_TFM") == "y"
        pm = pm_config.exists()

        if pm:
            header_size = find_in_config(pm_config, "PM_MCUBOOT_PAD_SIZE")
            slot_size = find_in_config(pm_config, "PM_MCUBOOT_PRIMARY_SIZE")
        else:
            if tfm:
                header_size = find_in_config(zephyr_config, "CONFIG_TFM_MCUBOOT_HEADER_SIZE")
            else:
                header_size = find_in_config(zephyr_config, "CONFIG_ROM_START_OFFSET")
            slot_size = str(get_code_slot(edt_data).regs[0].size)
        imgtool_params = ImgtoolParams(
            align=find_in_config(zephyr_config, "CONFIG_MCUBOOT_FLASH_WRITE_BLOCK_SIZE"),
            header_size=header_size,
            slot_size=slot_size,
            version=find_in_config(zephyr_config, "CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION"),
            pad_header=pm or tfm,
        )

        return cls(
            build_dir=build_dir,
            app_build_dir=app_build_dir,
            zephyr_base=zephyr_base,  # type: ignore
            zephyr_config=zephyr_config,
            pm_config=pm_config,
            sysbuild=sysbuild,
            tfm=tfm,
            pm=pm,
            imgtool_params=imgtool_params,
        )

    def __post_init__(self):
        """Post-initialization to update paths and imgtool parameters."""
        self._update_paths()
        self._update_imgtool()

    def _update_paths(self) -> None:
        """Update internal paths for MCUboot, app, and netcore images."""
        self.mcuboot_dir = Path(self.zephyr_base).parent / "bootloader" / "mcuboot"
        if self.sysbuild:
            if self.tfm:
                self.app_to_sign = self.app_build_dir / "zephyr" / "tfm_merged.hex"
            else:
                self.app_to_sign = self.app_build_dir / "zephyr" / "zephyr.bin"
            sysbuild_config = self.build_dir / "zephyr" / ".config"
            self.net_core_name = find_in_config(sysbuild_config, "SB_CONFIG_NETCORE_IMAGE_NAME")
            self.netcore_to_sign = self.build_dir / f"signed_by_b0_{self.net_core_name}.bin"
            if self.pm:
                self.mcuboot_secondary_app_to_sign = (
                    self.build_dir / "mcuboot_secondary_app" / "zephyr" / "zephyr.bin"
                )
            else:
                slot1_variant_dir = self.app_build_dir.with_name(
                    self.app_build_dir.name + "_slot1_variant"
                )
                self.mcuboot_secondary_app_to_sign = slot1_variant_dir / "zephyr" / "zephyr.bin"
        else:
            self.app_to_sign = self.build_dir / "zephyr" / "app_to_sign.bin"
            self.netcore_to_sign = self.build_dir / "zephyr" / "net_core_app_to_sign.bin"
            self.mcuboot_secondary_app_to_sign = (
                self.build_dir / "zephyr" / "mcuboot_secondary_app_to_sign.bin"
            )

    def _update_imgtool(self) -> None:
        """Update imgtool parameters based on build configuration."""
        self.imgtool_params.tool_path = str(self.mcuboot_dir / "scripts" / "imgtool.py")
        if self.sysbuild:
            self.imgtool_params.key_file = find_in_config(
                self.zephyr_config, "CONFIG_MCUBOOT_SIGNATURE_KEY_FILE"
            )  # type: ignore
            self.imgtool_params.encryption_key_file = (
                find_in_config(self.zephyr_config, "CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE") or None  # type: ignore
            )
            self._update_imgtool_next()
        else:
            mcuboot_config = self.build_dir / "mcuboot" / "zephyr" / ".config"
            key_filename = find_in_config(mcuboot_config, "CONFIG_BOOT_SIGNATURE_KEY_FILE")
            self.imgtool_params.key_file = self.mcuboot_dir / key_filename
        self._update_rom_fixed()

    def _update_rom_fixed(self) -> None:
        """Set --rom-fixed for imgtool when the build uses load-address image matching."""
        if (
            find_in_config(self.zephyr_config, "CONFIG_NCS_MCUBOOT_IMGTOOL_SET_ROM_FIXED_ADDRESS")
            != "y"
        ):
            return

        if not self.pm:
            edt_data = self.app_build_dir / "zephyr" / "edt.pickle"
            self.imgtool_params.rom_fixed = get_code_partition_address(edt_data)

    def update_params_for_netcore(self) -> None:
        """Update imgtool parameters for netcore image slot size."""
        if not self.sysbuild:
            self.imgtool_params.slot_size = find_in_config(
                self.pm_config, "PM_MCUBOOT_SECONDARY_1_SIZE"
            )
        elif self.pm_config.exists():
            cpunet_pm_config = self.build_dir / "pm_CPUNET.config"
            self.imgtool_params.slot_size = find_in_config(
                cpunet_pm_config, f"PM_{self.net_core_name.upper()}_SIZE"
            )
        else:
            edt_data = self.build_dir / self.net_core_name / "zephyr" / "edt.pickle"
            self.imgtool_params.slot_size = str(get_edt_node(edt_data, "s0_partition").regs[0].size)

    def _update_imgtool_next(self) -> None:
        """Update imgtool parameters for advanced signature and compression options."""
        sysbuild_config = self.build_dir / "zephyr" / ".config"
        if find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT"):
            self.imgtool_params.compression = "lzma2armthumb"

        if find_in_config(sysbuild_config, "SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE"):
            self.imgtool_params.pure = True
        elif find_in_config(sysbuild_config, "SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512"):
            self.imgtool_params.sha = 512
        if find_in_config(
            sysbuild_config, "SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519"
        ) and find_in_config(sysbuild_config, "SB_CONFIG_SOC_SERIES_NRF54L"):
            self.imgtool_params.hmac_sha = 512

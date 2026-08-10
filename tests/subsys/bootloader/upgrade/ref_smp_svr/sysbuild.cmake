#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

# FW_INFO is enabled in MCUboot when NSIB validates MCUboot, or when the hardware downgrade
# prevention uses the secure boot storage (all SoCs except nRF54H).
if(SB_CONFIG_BOOTLOADER_MCUBOOT AND (SB_CONFIG_SECURE_BOOT_APPCORE OR
   (SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION AND NOT SB_CONFIG_SOC_SERIES_NRF54H)))
  add_overlay_config(mcuboot ${APP_DIR}/sysbuild/mcuboot/fw_info.conf)
endif()

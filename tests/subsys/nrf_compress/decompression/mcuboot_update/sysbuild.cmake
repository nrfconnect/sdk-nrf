#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

ExternalZephyrProject_Add(
  APPLICATION compressed_app
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/compressed_app
)

set_config_bool(compressed_app CONFIG_BOOTLOADER_MCUBOOT "${SB_CONFIG_BOOTLOADER_MCUBOOT}")
set_config_string(compressed_app CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
                  "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}"
)
set_config_bool(compressed_app CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY y)
set(compressed_app_SIGNING_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/modified_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
set_config_bool(compressed_app CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED y)

if((SB_CONFIG_SOC_SERIES_NRF54L OR SB_CONFIG_SOC_SERIES_NRF54H) AND SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
  set_config_bool(compressed_app CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_ED25519 y)
  set_config_bool(compressed_app CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512 y)
  set_config_bool(compressed_app CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE n)
endif()

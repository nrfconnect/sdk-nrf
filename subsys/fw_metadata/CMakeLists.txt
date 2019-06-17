#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
if (CONFIG_FW_METADATA OR CONFIG_IS_SECURE_BOOTLOADER)
  zephyr_library()
  zephyr_library_sources(fw_metadata.c)
endif ()

# The following creates variables with the magic numbers used for firmware
# metadata as comma-separated lists of numbers.

math(EXPR
  MAGIC_COMPATIBILITY_INFO
  "(${CONFIG_FW_INFO_VERSION}) |
   (${CONFIG_FW_HARDWARE_ID} << 8) |
   (${CONFIG_FW_INFO_CRYPTO_ID} << 16) |
   (${CONFIG_FW_CUSTOM_COMPATIBILITY_ID} << 24)"
  )

set(FIRMWARE_INFO_MAGIC   "${CONFIG_FW_MAGIC_COMMON},${CONFIG_FW_MAGIC_FIRMWARE_INFO},${MAGIC_COMPATIBILITY_INFO}")
set(ABI_INFO_MAGIC        "${CONFIG_FW_MAGIC_COMMON},${CONFIG_FW_MAGIC_ABI_INFO},${MAGIC_COMPATIBILITY_INFO}")

zephyr_compile_definitions(
  FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
  ABI_INFO_MAGIC=${ABI_INFO_MAGIC}
  )

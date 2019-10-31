#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
if (CONFIG_FW_INFO)
  zephyr_library()
  zephyr_library_sources(fw_info.c)
  zephyr_linker_sources(RODATA ext_apis.ld)
endif ()

zephyr_linker_sources_ifdef(CONFIG_FW_INFO
  TEXT_START
  SORT_KEY ${CONFIG_FW_INFO_OFFSET}
  fw_info.ld)

# The following creates variables with the magic numbers used for firmware
# metadata as comma-separated lists of numbers.

math(EXPR
  MAGIC_COMPATIBILITY_INFO
  "(${CONFIG_FW_INFO_VERSION}) |
   (${CONFIG_FW_INFO_HARDWARE_ID} << 8) |
   (${CONFIG_FW_INFO_CRYPTO_ID} << 16) |
   (${CONFIG_FW_INFO_MAGIC_COMPATIBILITY_ID} << 24)"
  )

set(FIRMWARE_INFO_MAGIC   "${CONFIG_FW_INFO_MAGIC_COMMON},${CONFIG_FW_INFO_MAGIC_FIRMWARE_INFO},${MAGIC_COMPATIBILITY_INFO}")
set(EXT_API_MAGIC         "${CONFIG_FW_INFO_MAGIC_COMMON},${CONFIG_FW_INFO_MAGIC_EXT_API},${MAGIC_COMPATIBILITY_INFO}")

zephyr_compile_definitions(
  FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
  EXT_API_MAGIC=${EXT_API_MAGIC}
  )

#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
if (CONFIG_FW_INFO)
  zephyr_library()
  zephyr_library_sources(fw_info.c)
  zephyr_linker_sources(RODATA ext_apis.ld)
endif ()

if (CONFIG_FW_INFO_API)
  zephyr_library()
endif()

zephyr_linker_sources_ifdef(CONFIG_FW_INFO
  ROM_START
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

if (CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DFIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
      -DEXT_API_MAGIC=${EXT_API_MAGIC}
  )
endif()

zephyr_compile_definitions(
  FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
  EXT_API_MAGIC=${EXT_API_MAGIC}
  )

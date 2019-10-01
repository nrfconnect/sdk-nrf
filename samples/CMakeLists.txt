#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SPM)
  if (CONFIG_SECURE_BOOT)
    # Inject fw_metadata kconfig option
    set(conf_path "${ZEPHYR_BASE}/../nrf/subsys/fw_metadata/fw_metadata.conf")
    add_overlay_config(${conf_path} spm)
  endif()

  zephyr_add_executable(spm require_build)
  if (${require_build})
    add_subdirectory(nrf9160/spm ${CMAKE_BINARY_DIR}/spm)
  endif ()
endif()

if (CONFIG_SECURE_BOOT)
  zephyr_add_executable(b0 require_build)
  if(${require_build})
    add_subdirectory(bootloader ${CMAKE_BINARY_DIR}/b0)
  endif ()
endif ()

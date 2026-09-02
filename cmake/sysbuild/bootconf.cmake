#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

set(bootconf_hex ${CMAKE_BINARY_DIR}/bootconf.hex)
set(bootconf_dependency)

if(SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES)
  set(bootconf_image b0)
  if(SB_CONFIG_PARTITION_MANAGER)
    set(bootconf_size $<TARGET_PROPERTY:partition_manager,PM_B0_SIZE>)
    set(bootconf_dependency ${APPLICATION_BINARY_DIR}/pm.config)
  else()
    dt_partition_size(bootconf_size LABEL b0_partition TARGET b0 REQUIRED)
  endif()
elseif(SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES)
  set(bootconf_image mcuboot)
  dt_partition_size(bootconf_size LABEL boot_partition TARGET mcuboot REQUIRED)
else()
  message(FATAL_ERROR "bootconf.cmake included without bootconf Kconfig enabled")
endif()

# bootconf.hex is only created when there are b0_partition or boot_partition, and
# sysbuild kconfig enables it.
if(NOT bootconf_size EQUAL 0)
  add_custom_command(OUTPUT ${bootconf_hex}
    COMMAND ${Python3_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/scripts/reglock.py
      --output ${bootconf_hex}
      --size ${bootconf_size}
      --soc ${SB_CONFIG_SOC}
    DEPENDS ${bootconf_dependency}
    VERBATIM
  )
else()
  # Whether we have this CMake invoked or not is controlled by paths in
  # scripts that invoke; it is expected to be called when an Immutable Bootloader is
  # build, which means that a partition for it also exists. If these
  # expectations are not met and somehow we have this part invoked, this means
  # that bootconf build has been invoked for something it can not handle.
  message(FATAL_ERROR "bootconf.hex has nothing to protect."
    "CMake path that should not have been taken?"
  )
endif()

add_custom_target(bootconf_target
  DEPENDS ${bootconf_hex}
)

if(SB_CONFIG_MERGED_HEX_FILES)
  set(board_target)
  sysbuild_get(board_target IMAGE ${bootconf_image} VAR CONFIG_BOARD_TARGET KCONFIG)
  string(REPLACE "/" "_" board_target ${board_target})
  string(REPLACE "@" "_" board_target ${board_target})

  set_property(GLOBAL APPEND
    PROPERTY sysbuild_merged_hex_dependencies_${board_target} bootconf_target
  )

  set(board_target)
endif()

if(SB_CONFIG_PARTITION_MANAGER)
  set_property(
    GLOBAL PROPERTY
    bootconf_PM_HEX_FILE
    ${bootconf_hex}
  )

  set_property(
    GLOBAL PROPERTY
    bootconf_PM_TARGET
    bootconf_target
  )
else()
  add_dependencies(${bootconf_image} bootconf_target)
endif()

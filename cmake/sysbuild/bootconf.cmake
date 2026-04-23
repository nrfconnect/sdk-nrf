#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

set(bootconf_hex ${CMAKE_BINARY_DIR}/bootconf.hex)
set(bootconf_dependency)

if(SB_CONFIG_PARTITION_MANAGER)
  set(bootconf_size $<TARGET_PROPERTY:partition_manager,PM_B0_SIZE>)
  set(bootconf_dependency ${APPLICATION_BINARY_DIR}/pm.config)
else()
  dt_partition_size(bootconf_size LABEL b0_partition TARGET b0 REQUIRED)
endif()

# bootconf.hex is only created when there is b0_partition, which
# indicates that b0 is used.
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
  # scripts that invoke; it is expected to be called when Secure Bootloader is
  # build, which means that the B0 partition for it also exists. If these
  # expectations are not met and somehow we have this part invoked, this means
  # that bootconf build has been invoked for something it can not handle.
  message(FATAL_ERROR "bootconf.hex has nothing to protect."
    "CMake path that should not have been taken?"
  )
endif()

add_custom_target(bootconf_target
  DEPENDS ${bootconf_hex}
)

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
  # Add the bootconf with b0 as bootconf is supposed to protect it.
  add_dependencies(b0 bootconf_target)
endif()

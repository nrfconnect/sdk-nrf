#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

set(bootconf_hex ${CMAKE_BINARY_DIR}/bootconf.hex)
set(bootconf_dependency)

dt_partition_size(bootconf_size LABEL b0_partition TARGET b0 REQUIRED)

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

if(SB_CONFIG_MERGED_HEX_FILES)
  set(board_target)
  sysbuild_get(board_target IMAGE b0 VAR CONFIG_BOARD_TARGET KCONFIG)
  string(REPLACE "/" "_" board_target ${board_target})
  string(REPLACE "@" "_" board_target ${board_target})

  set_property(GLOBAL APPEND
    PROPERTY sysbuild_merged_hex_dependencies_${board_target} bootconf_target
  )

  set(board_target)
endif()

# Add the bootconf with b0 as bootconf is supposed to protect it.
add_dependencies(b0 bootconf_target)

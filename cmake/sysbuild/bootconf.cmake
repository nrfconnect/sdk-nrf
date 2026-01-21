#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(setup_bootconf_data)
  if(SB_CONFIG_PARTITION_MANAGER)
    add_custom_target(bootconf_target
      DEPENDS ${CMAKE_BINARY_DIR}/bootconf.hex
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/bootconf.hex
      COMMAND ${Python3_EXECUTABLE}
        ${ZEPHYR_NRF_MODULE_DIR}/scripts/reglock.py
        --output ${CMAKE_BINARY_DIR}/bootconf.hex
        --size $<TARGET_PROPERTY:partition_manager,PM_B0_SIZE>
        --soc ${SB_CONFIG_SOC}
      DEPENDS ${APPLICATION_BINARY_DIR}/pm.config
      VERBATIM
    )

    set_property(
      GLOBAL PROPERTY
      bootconf_PM_HEX_FILE
      ${CMAKE_BINARY_DIR}/bootconf.hex
    )

    set_property(
      GLOBAL PROPERTY
      bootconf_PM_TARGET
      bootconf_target
    )
  endif()
endfunction()

setup_bootconf_data()

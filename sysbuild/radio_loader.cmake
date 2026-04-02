#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Radio Loader sysbuild integration
# This file handles automatic integration of the radio_loader application

if(SB_CONFIG_NRF_RADIO_LOADER)
  # Add radio_loader as an external Zephyr project
  ExternalZephyrProject_Add(
    APPLICATION radio_loader
      SOURCE_DIR "${ZEPHYR_NRF_MODULE_DIR}/samples/nrf54h20/radio_loader"
      BOARD ${SB_CONFIG_NRF_RADIO_LOADER_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
  )

  set_target_properties(radio_loader PROPERTIES
    IMAGE_CONF_SCRIPT ${ZEPHYR_BASE}/share/sysbuild/image_configurations/MAIN_image_default.cmake
  )

  UpdateableImage_Add(APPLICATION radio_loader)

  # radio_loader is not flashed as its own domain — its hex is merged into the
  # netcore (ipc_radio) hex so that a single flash of the radio domain programs
  # both the loader (MRAM) and the BT controller firmware (MRAM, copied to TCM).
  set_target_properties(radio_loader PROPERTIES BUILD_ONLY true)

  if(NOT SB_CONFIG_NETCORE_NONE)
    add_dependencies(radio_loader ${SB_CONFIG_NETCORE_IMAGE_NAME})

    ExternalProject_Get_Property(radio_loader BINARY_DIR)
    set(RADIO_LOADER_BINARY_DIR ${BINARY_DIR})
    ExternalProject_Get_Property(${SB_CONFIG_NETCORE_IMAGE_NAME} BINARY_DIR)
    set(NETCORE_BINARY_DIR ${BINARY_DIR})

    add_custom_target(merge_radio_loader ALL
      COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
        -o ${NETCORE_BINARY_DIR}/zephyr/zephyr.hex
        --overlap replace
        ${NETCORE_BINARY_DIR}/zephyr/zephyr.hex
        ${RADIO_LOADER_BINARY_DIR}/zephyr/zephyr.hex
      DEPENDS radio_loader ${SB_CONFIG_NETCORE_IMAGE_NAME}
      COMMENT "Merging radio_loader into ${SB_CONFIG_NETCORE_IMAGE_NAME} hex"
    )
  endif()
endif()

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
  # Note: Memory map configuration should be provided by the user project
  # at the project level in sysbuild directory.
  # This overlay should define partitions and nodelabels for the loader
endif()

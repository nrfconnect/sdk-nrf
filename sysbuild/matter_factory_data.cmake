#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_ZEPHYR_CONNECTEDHOMEIP_MODULE AND SB_CONFIG_MATTER AND
  SB_CONFIG_MATTER_FACTORY_DATA_GENERATE AND NOT SB_CONFIG_PARTITION_MANAGER
)
  include(image_flasher.cmake)
  add_image_flasher(NAME matter_factory_data HEX_FILE "${CMAKE_BINARY_DIR}/factory_data.hex")
endif()

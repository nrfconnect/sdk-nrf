#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Manufacturing Application sysbuild integration
# This file handles automatic integration of the manufacturing application

if(SB_CONFIG_NRF_MANUFACTURING_APP)
  # Add manufacturing application as an external Zephyr project
  ExternalZephyrProject_Add(
    APPLICATION "manufacturing_app"
      SOURCE_DIR "${SB_CONFIG_NRF_MANUFACTURING_APP_IMAGE_PATH}"
      BOARD ${SB_CONFIG_NRF_MANUFACTURING_APP_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
  )

  set_target_properties(manufacturing_app PROPERTIES
    IMAGE_CONF_SCRIPT ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/image_configurations/manufacturing_app_image_default.cmake
  )

  if(SB_CONFIG_PARTITION_MANAGER)
    set_target_properties(${image} PROPERTIES BUILD_ONLY true)
  endif()

  UpdateableImage_Add(APPLICATION manufacturing_app)
endif()

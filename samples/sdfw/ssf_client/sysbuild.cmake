#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT "${SB_CONFIG_RADIO_IMAGE_BOARD}" STREQUAL "")
  ExternalZephyrProject_Add(
    APPLICATION rad_image
    SOURCE_DIR ${APP_DIR}/rad_image
    BOARD ${SB_CONFIG_RADIO_IMAGE_BOARD}
  )

  add_dependencies(ssf_client rad_image)
  sysbuild_add_dependencies(FLASH ssf_client rad_image)
endif()

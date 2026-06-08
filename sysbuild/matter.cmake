
#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_ZEPHYR_CONNECTEDHOMEIP_MODULE AND SB_CONFIG_MATTER AND
  SB_CONFIG_MATTER_FACTORY_DATA_GENERATE AND NOT SB_CONFIG_PARTITION_MANAGER
)
  include(image_flasher.cmake)
  add_image_flasher(
    NAME matter_factory_data
    HEX_FILE "${CMAKE_BINARY_DIR}/matter_factory_data/zephyr/factory_data.hex"
  )

  if(SB_CONFIG_MERGED_HEX_FILES)
    if(DEFINED BOARD_REVISION)
      set(board_target "${BOARD}@${BOARD_REVISION}${BOARD_QUALIFIERS}")
    else()
      set(board_target "${BOARD}${BOARD_QUALIFIERS}")
    endif()

    string(REPLACE "/" "_" board_target ${board_target})
    string(REPLACE "." "_" board_target ${board_target})
    string(REPLACE "@" "_" board_target ${board_target})

    set_property(GLOBAL APPEND
      PROPERTY sysbuild_merged_hex_dependencies_${board_target} factory_data
    )

    set(board_target)
  endif()
endif()

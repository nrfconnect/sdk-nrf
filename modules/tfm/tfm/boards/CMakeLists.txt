#-------------------------------------------------------------------------------
# Copyright (c) 2021, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# NOTE: All code in this folder and subfolders is run as part of the TF-M
#       build, not in a Zephyr context.

set(NRF_BOARD_SELECTED True)

set(partition_includes
  partition
  ${CMAKE_BINARY_DIR}/../zephyr/include/generated
  )

set(board_includes
  board
  ${CMAKE_BINARY_DIR}/../zephyr/misc/generated/syscalls_links/include
  ${ZEPHYR_BASE}/include
  )

target_include_directories(platform_region_defs INTERFACE ${partition_includes})

target_include_directories(platform_s
  PUBLIC ${partition_includes} ${board_includes}
  )

target_include_directories(platform_ns PUBLIC ${board_includes})

if(BL2)
  target_include_directories(platform_bl2
    PUBLIC ${partition_includes}
    PRIVATE ${board_includes}
    )
endif()

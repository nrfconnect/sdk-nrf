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
    PUBLIC
      ${partition_includes}
    PRIVATE
      ${board_includes}
    )
  target_sources(platform_bl2
    PRIVATE
      ${CMAKE_CURRENT_LIST_DIR}/common/boot_hal.c
  )

  target_link_libraries(platform_bl2 PRIVATE platform_cc3xx)

endif()

if(TFM_PARTITION_PLATFORM)
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/src)
  set(nrf_dir ${CMAKE_CURRENT_LIST_DIR}/../../../../)

  target_include_directories(platform_s
    PUBLIC
    include
    ${nrf_dir}/include/tfm
    )

  target_sources(platform_s
    PRIVATE
    ${src_dir}/tfm_platform_system.c
    ${src_dir}/tfm_ioctl_s_api.c
    )

  target_include_directories(platform_ns
    PUBLIC
    include
    ${nrf_dir}/include/tfm
    )

  target_sources(platform_ns
    PRIVATE
    ${src_dir}/tfm_ioctl_ns_api.c
    )
endif()

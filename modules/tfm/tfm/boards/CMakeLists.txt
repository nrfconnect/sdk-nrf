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

target_compile_definitions(platform_s
  PRIVATE
  $<$<BOOL:${PLATFORM_DUMMY_CRYPTO_KEYS}>:PLATFORM_DUMMY_CRYPTO_KEYS>
  )

target_include_directories(platform_s
  PUBLIC ${partition_includes} ${board_includes}
  ${NRF_DIR}/include
  )

target_sources(platform_s
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/common/tfm_hal_platform.c
  )

if (${TFM_PARTITION_CRYPTO}
      AND (NOT ${CRYPTO_KEY_DERIVATION_MODULE_DISABLED})
      AND (NOT ${PLATFORM_DUMMY_CRYPTO_KEYS}))
  target_sources(platform_s
    PRIVATE
      ${NRF_DIR}/lib/hw_unique_key/hw_unique_key.c
      ${NRF_DIR}/lib/hw_unique_key/hw_unique_key_kmu.c
      ${CMAKE_CURRENT_LIST_DIR}/common/crypto_keys.c
  )
endif()

target_link_libraries(platform_s PRIVATE platform_cc3xx)
target_compile_definitions(platform_s PUBLIC CONFIG_HW_UNIQUE_KEY CONFIG_HW_UNIQUE_KEY_RANDOM)

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

  target_include_directories(platform_s
    PUBLIC
    include
    ${NRF_DIR}/include/tfm
    ${NRF_DIR}/include
    )

  target_sources(platform_s
    PRIVATE
    ${src_dir}/tfm_platform_system.c
    ${src_dir}/tfm_ioctl_s_api.c
    )

  target_include_directories(platform_ns
    PUBLIC
    include
    ${NRF_DIR}/include/tfm
    )

  target_sources(platform_ns
    PRIVATE
    ${src_dir}/tfm_ioctl_ns_api.c
    )
endif()

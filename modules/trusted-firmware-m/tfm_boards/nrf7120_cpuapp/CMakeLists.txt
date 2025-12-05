#
# Copyright (c) 2025, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(NRF_BOARD_SELECTED True)

add_subdirectory(${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/common/nrf7120 nrf7120)

add_subdirectory(.. tfm_board)

target_include_directories(platform_s
  PUBLIC
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_security/src/drivers/cracen/cracenpsa/include
  )

target_sources(platform_s
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/soc/nordic/nrf71/soc.c
  )

target_include_directories(platform_s
  PRIVATE
  ${ZEPHYR_BASE}/modules/cmsis/
  ${ZEPHYR_NRF_MODULE_DIR}/soc/nordic/nrf71
  ${ZEPHYR_BASE}/soc/nordic/common
  )

install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(FILES       ../common/config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/)

install(DIRECTORY   ${Trusted\ Firmware\ M_SOURCE_DIR}/platform/ext/target/nordic_nrf/nrf7120dk_nrf7120_cpuapp/tests
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
)

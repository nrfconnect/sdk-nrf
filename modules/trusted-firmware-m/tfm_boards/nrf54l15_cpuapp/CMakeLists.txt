#-------------------------------------------------------------------------------
# Copyright (c) 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

set(NRF_BOARD_SELECTED True)

add_subdirectory(${OOT_54L_PLATFORM_PATH} nrf54l15)

add_subdirectory(.. tfm_board)


install(FILES       ${CMAKE_CURRENT_LIST_DIR}/ns/cpuarch_ns.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}
        RENAME      cpuarch.cmake)

install(FILES       config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

install(FILES       ../common/config.cmake
        DESTINATION ${INSTALL_PLATFORM_NS_DIR}/../common/)

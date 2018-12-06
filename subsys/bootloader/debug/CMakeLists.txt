#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(EXTS ${ZEPHYR_BASE}/ext)
set(SEGGER ${EXTS}/debug/segger)

add_library(debuglib STATIC printk.c)
target_link_libraries(debuglib PRIVATE zephyr_interface)

add_subdirectory_ifdef(CONFIG_SB_DEBUG_PORT_UART uart)
target_link_libraries_ifdef(CONFIG_SB_DEBUG_PORT_UART debuglib PRIVATE uart)

add_subdirectory_ifdef(CONFIG_SB_DEBUG_PORT_SEGGER_RTT rtt)
target_link_libraries_ifdef(CONFIG_SB_DEBUG_PORT_SEGGER_RTT debuglib PRIVATE segger_rtt)

target_include_directories_ifdef(CONFIG_SB_DEBUG_PORT_SEGGER_RTT debuglib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/debug)
target_include_directories_ifdef(CONFIG_SB_DEBUG_PORT_SEGGER_RTT debuglib PUBLIC ${SEGGER}/rtt)

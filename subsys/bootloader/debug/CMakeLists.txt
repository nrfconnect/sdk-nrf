#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(EXTS ${ZEPHYR_BASE}/ext)
set(SEGGER ${EXTS}/debug/segger)

zephyr_include_directories(
  ../include/debug
  )

add_subdirectory_ifdef(CONFIG_SB_DEBUG_PORT_UART uart)
add_subdirectory_ifdef(CONFIG_SB_DEBUG_PORT_SEGGER_RTT rtt)

zephyr_library()
zephyr_library_sources(printk.c)

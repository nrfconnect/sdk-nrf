#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(EXTS ${ZEPHYR_BASE}/ext)
set(SEGGER ${EXTS}/debug/segger)

add_library(segger_rtt STATIC
  ${SEGGER}/rtt/SEGGER_RTT.c
  ${SEGGER}/rtt/SEGGER_RTT_printf.c
)

target_link_libraries(segger_rtt PRIVATE libc)
target_link_libraries(segger_rtt PRIVATE zephyr_interface)

target_include_directories(segger_rtt PUBLIC ../../include/debug)
target_include_directories(segger_rtt PUBLIC ${SEGGER}/rtt)

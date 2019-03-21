#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(EXTS ${ZEPHYR_BASE}/ext)
set(SEGGER ${EXTS}/debug/segger)

zephyr_library()

zephyr_library_sources(
  ${SEGGER}/rtt/SEGGER_RTT.c
  ${SEGGER}/rtt/SEGGER_RTT_printf.c
  )

zephyr_library_include_directories(../../include/debug)
zephyr_include_directories(${SEGGER}/rtt)

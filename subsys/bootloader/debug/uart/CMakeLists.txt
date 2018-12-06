#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(HALDRIVERS ${ZEPHYR_BASE}/ext/hal/nordic/nrfx/drivers/src)

add_library(uart ${HALDRIVERS}/nrfx_uarte.c uart.c)

target_compile_definitions(uart PRIVATE NRFX_UARTE0_ENABLED)
target_compile_definitions(uart PRIVATE NRFX_UARTE_ENABLED)

target_link_libraries(uart PRIVATE zephyr_interface)
target_link_libraries(uart PRIVATE libc)

target_include_directories(uart PRIVATE BEFORE .)

#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(HALDRIVERS ${ZEPHYR_BASE}/ext/hal/nordic/nrfx/drivers/src)

#TODO get this from nrfx directly instead of doing it again

zephyr_library()

zephyr_library_sources(
  ${HALDRIVERS}/nrfx_uarte.c
  uart.c
  )

zephyr_library_compile_definitions(
  NRFX_UARTE0_ENABLED
  NRFX_UARTE_ENABLED
  )

zephyr_library_include_directories(.)

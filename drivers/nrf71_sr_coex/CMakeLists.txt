#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(nrf71_sr_coex)

zephyr_library_sources(src/nrf71_sr_coex.c)
zephyr_library_sources(src/nrf71_sr_coex_cm.c)
# Access to the coexistence firmware interface headers.
zephyr_library_include_directories(
  include
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/fw_if
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/common/fw_if
)

zephyr_include_directories(include)

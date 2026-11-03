#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(nrf71_sr_coex)

zephyr_library_sources(
  src/nrf71_sr_coex.c
  src/nrf71_sr_coex_cm.c
)
zephyr_library_sources_ifdef(CONFIG_NRF71_SR_COEX_SHELL src/nrf71_sr_coex_shell.c)

# The coexistence firmware interface headers are private to the drivers.
zephyr_library_include_directories(
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/common/fw_if
)

#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(nrf71_sr_coex)

zephyr_library_sources(src/nrf71_sr_coex.c)
zephyr_library_sources(src/nrf71_sr_coex_cm.c)

# Private access to the coexistence firmware interface headers for the CD2CM /
# CD-to-SR message structures (nrf71_coex_if.h, nrf71_cd_sr_if.h and the
# pack_def.h they pull in). The public API of this driver does not expose these
# types.
zephyr_library_include_directories(
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/inc/fw_if
  ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71/osal/hw_if/hal/inc
)

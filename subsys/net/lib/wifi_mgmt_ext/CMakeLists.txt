#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(wifi_mgmt_ext.c)

zephyr_library_include_directories(${NCS_DIR}/modules/lib/hostap/wpa_supplicant/)
zephyr_library_include_directories(${NCS_DIR}/modules/lib/hostap/src/)
zephyr_library_include_directories(${NCS_DIR}/modules/lib/hostap/src/utils)

zephyr_include_directories(
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/zephyr/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/utils/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/os_if/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/bus_if/bus/qspi/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/bus_if/bal/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/fw_if/umac_if/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/fw_load/mips/fw/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/osal/hw_if/hal/inc
	${NCS_DIR}/main/nrf/drivers/wifi/nrf700x/zephyr/src/qspi/inc
)

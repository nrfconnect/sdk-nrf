#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_shell)

set(WIFI_NRF_DRV_BASE ${WEST_TOPDIR}/nrf/drivers/wifi/nrf700x)
if(CONFIG_WPA_SUPP)
set(WPA_SUPP_BASE ${WEST_TOPDIR}/modules/lib/hostap)
endif()

# TODO: Remove this hack
target_include_directories(mbedcrypto_common
	INTERFACE
	${CMAKE_SOURCE_DIR}
	${ZEPHYR_BASE}/subsys/net/ip
	${WIFI_NRF_DRV_BASE}/osal/os_if/inc
	${WIFI_NRF_DRV_BASE}/zephyr/inc
	${WIFI_NRF_DRV_BASE}/osal/fw_if/umac_if/inc
	${WIFI_NRF_DRV_BASE}/osal/fw_if/umac_if/inc/fw
	${WIFI_NRF_DRV_BASE}/osal/hw_if/hal/inc/fw
if(CONFIG_WPA_SUPP)
	${WPA_SUPP_BASE}/wpa_supplicant
	${WPA_SUPP_BASE}/src
	${WPA_SUPP_BASE}/zephyr/src
endif()
)

target_compile_definitions_ifdef(CONFIG_WPA_SUPP_CRYPTO
	app
	PRIVATE
	CONFIG_WEP
)

target_sources(app PRIVATE
	src/main.c
	src/supp_shell.c
)

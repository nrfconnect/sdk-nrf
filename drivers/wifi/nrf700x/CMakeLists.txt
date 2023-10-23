#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

set(OS_AGNOSTIC_BASE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_wifi)

zephyr_include_directories(
	inc
	${OS_AGNOSTIC_BASE}/utils/inc
	${OS_AGNOSTIC_BASE}/os_if/inc
	${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/inc
	${OS_AGNOSTIC_BASE}/bus_if/bal/inc
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc
	${OS_AGNOSTIC_BASE}/fw_load/mips/fw/inc
	${OS_AGNOSTIC_BASE}/hw_if/hal/inc
	src/qspi/inc
	# for net_sprint_ll_addr
	${ZEPHYR_BASE}/subsys/net/ip
	${OS_AGNOSTIC_BASE}/hw_if/hal/inc/fw
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/fw
)

zephyr_include_directories_ifdef(CONFIG_NRF700X_RADIO_TEST
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/radio_test
)

zephyr_include_directories_ifndef(CONFIG_NRF700X_RADIO_TEST
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/default
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_BT_COEX
	src/coex.c
)

zephyr_library_sources(
	${OS_AGNOSTIC_BASE}/os_if/src/osal.c
	${OS_AGNOSTIC_BASE}/utils/src/list.c
	${OS_AGNOSTIC_BASE}/utils/src/queue.c
	${OS_AGNOSTIC_BASE}/utils/src/util.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_api.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_fw_patch_loader.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_interrupt.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_mem.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_reg.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/hpqm.c
	${OS_AGNOSTIC_BASE}/hw_if/hal/src/pal.c
	${OS_AGNOSTIC_BASE}/bus_if/bal/src/bal.c
	${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/src/qspi.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/cmd.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/event.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_api_common.c
	src/shim.c
	src/work.c
	src/timer.c
	src/fmac_main.c
	src/fw_load.c
	src/qspi/src/device.c
	src/qspi/src/rpu_hw_if.c
	src/qspi/src/ficr_prog.c
)

zephyr_library_sources_ifndef(CONFIG_NRF700X_RADIO_TEST
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/rx.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_vif.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
	src/net_if.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/default/fmac_api.c
)

zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
	src/wifi_mgmt_scan.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_RADIO_TEST
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/radio_test/fmac_api.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_DATA_TX
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/tx.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_STA_MODE
	src/wpa_supp_if.c
	src/wifi_mgmt.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_AP_MODE
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_ap.c
)


# Without WPA supplicant we only support scan
zephyr_library_sources_ifdef(CONFIG_NRF700X_STA_MODE
	src/wpa_supp_if.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
	${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_QSPI
	src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_SPI
	src/qspi/src/spi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_UTIL
	src/wifi_util.c
)

if (CONFIG_NRF_WIFI_PATCHES_EXT_FLASH)
# Run patches from the external flash (XIP). No need to copy.
zephyr_code_relocate(FILES src/fw_load.c LOCATION EXTFLASH_RODATA NOCOPY)
endif()

# These are XIP related anomalies and aren't applicable for nRF7002 and cause
# throughput issues.
zephyr_compile_definitions_ifdef(CONFIG_NRF700X_ON_QSPI
	-DNRF53_ERRATA_43_ENABLE_WORKAROUND=0
	-DNRF52_ERRATA_215_ENABLE_WORKAROUND=0
)

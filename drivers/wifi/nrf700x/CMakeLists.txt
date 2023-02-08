#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_include_directories(
	zephyr/inc
	osal/utils/inc
	osal/os_if/inc
	osal/bus_if/bus/qspi/inc
	osal/bus_if/bal/inc
	osal/fw_if/umac_if/inc
	osal/fw_load/mips/fw/inc
	osal/hw_if/hal/inc
	zephyr/src/qspi/inc
	# for net_sprint_ll_addr
	${ZEPHYR_BASE}/subsys/net/ip
	osal/hw_if/hal/inc/fw
	osal/fw_if/umac_if/inc/fw
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_BT_COEX
	zephyr/src/zephyr_coex.c
)

zephyr_library_sources(
	osal/os_if/src/osal.c
	osal/utils/src/list.c
	osal/utils/src/queue.c
	osal/utils/src/util.c
	osal/hw_if/hal/src/hal_api.c
	osal/hw_if/hal/src/hal_fw_patch_loader.c
	osal/hw_if/hal/src/hal_interrupt.c
	osal/hw_if/hal/src/hal_mem.c
	osal/hw_if/hal/src/hal_reg.c
	osal/hw_if/hal/src/hpqm.c
	osal/hw_if/hal/src/pal.c
	osal/bus_if/bal/src/bal.c
	osal/bus_if/bus/qspi/src/qspi.c
	osal/fw_if/umac_if/src/cmd.c
	osal/fw_if/umac_if/src/event.c
	osal/fw_if/umac_if/src/fmac_api.c
	zephyr/src/shim.c
	zephyr/src/zephyr_work.c
	zephyr/src/timer.c
	zephyr/src/zephyr_fmac_main.c
	zephyr/src/qspi/src/device.c
	zephyr/src/qspi/src/rpu_hw_if.c
	zephyr/src/qspi/src/ficr_prog.c
)

zephyr_library_sources_ifndef(CONFIG_NRF700X_RADIO_TEST
	osal/fw_if/umac_if/src/rx.c
	osal/fw_if/umac_if/src/fmac_vif.c
	osal/fw_if/umac_if/src/fmac_util.c
	zephyr/src/zephyr_wifi_mgmt.c
	zephyr/src/zephyr_net_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_DATA_TX
	osal/fw_if/umac_if/src/tx.c
	osal/fw_if/umac_if/src/fmac_peer.c
)

zephyr_library_sources_ifdef(CONFIG_WPA_SUPP
	zephyr/src/zephyr_wpa_supp_if.c
	osal/fw_if/umac_if/src/fmac_peer.c
	osal/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(NRF700X_AP_MODE
	osal/fw_if/umac_if/src/fmac_ap.c
)


# Without WPA supplicant we only support scan
zephyr_library_sources_ifdef(CONFIG_WPA_SUPP
	zephyr/src/zephyr_wpa_supp_if.c
	osal/fw_if/umac_if/src/fmac_peer.c
	osal/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_QSPI
	zephyr/src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_SPI
	zephyr/src/qspi/src/spi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_UTIL
	zephyr/src/zephyr_wifi_util.c
)

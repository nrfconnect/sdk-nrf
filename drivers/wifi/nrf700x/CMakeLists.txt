#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_compile_definitions(
	SOC_CALDER
	RPU_SUPPORT_WAPI
	RPU_VHT_SUPPORT
	RPU_HE_SUPPORT
	RPU_BSS_DB_SUPPORT
	RPU_32_BIT_DMA_SUPPORT
	RPU_CONFIG_FMAC
	REG_MODE_SUPPORT
	REG_DEBUG_MODE_SUPPORT
	OFFLINE_MODE
	SHELIAK_SOC	# SOC
	ASICBUILD	# SOC
	C0_CHIP
)

zephyr_include_directories(
	zephyr/inc
	osal/utils/inc
	osal/os_if/inc
	osal/bus_if/bus/qspi/inc
	osal/bus_if/bal/inc
	osal/fw_if/umac_if/inc
	osal/fw_if/umac_if/inc/fw
	osal/fw_load/mips/fw/inc
	osal/hw_if/hal/inc
	osal/hw_if/hal/inc/fw
	zephyr/src/qspi/inc
)

zephyr_library_sources(
	osal/os_if/src/osal.c
	osal/utils/src/list.c
	osal/utils/src/queue.c
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
	osal/fw_if/umac_if/src/fmac_ap.c
	osal/fw_if/umac_if/src/fmac_peer.c
	osal/fw_if/umac_if/src/fmac_util.c
	osal/fw_if/umac_if/src/fmac_vif.c
	osal/fw_if/umac_if/src/fmac_api.c
	osal/fw_if/umac_if/src/rx.c
	osal/fw_if/umac_if/src/tx.c
	zephyr/src/shim.c
	zephyr/src/zephyr_work.c
	zephyr/src/timer.c
	zephyr/src/util.c
	zephyr/src/zephyr_disp_scan.c
	zephyr/src/zephyr_fmac_main.c
	zephyr/src/zephyr_net_if.c
	zephyr/src/zephyr_wpa_supp_if.c
	zephyr/src/qspi/src/device.c
	zephyr/src/qspi/src/rpu_hw_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_QSPI
	zephyr/src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_SPI
	zephyr/src/qspi/src/spi_if.c
)

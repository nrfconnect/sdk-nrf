/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing FW load functions for Zephyr.
 *
 * For NRF QSPI NOR special handling is needed for this file as all RODATA of
 * this file is stored in external flash, so, any use of RODATA has to be protected
 * by disabling XIP and enabling it again after use. This means no LOG_* macros
 * (buffered) or buffered printk can be used in this file, else it will crash.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
#include <zephyr/drivers/flash/nrf_qspi_nor.h>
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH */

#include <zephyr_fmac_main.h>
#include <rpu_fw_patches.h>

enum wifi_nrf_status wifi_nrf_fw_load(void *rpu_ctx)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_fmac_fw_info fw_info;
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH */

	memset(&fw_info, 0, sizeof(fw_info));
	fw_info.lmac_patch_pri.data = wifi_nrf_lmac_patch_pri_bimg;
	fw_info.lmac_patch_pri.size = sizeof(wifi_nrf_lmac_patch_pri_bimg);
	fw_info.lmac_patch_sec.data = wifi_nrf_lmac_patch_sec_bin;
	fw_info.lmac_patch_sec.size = sizeof(wifi_nrf_lmac_patch_sec_bin);
	fw_info.umac_patch_pri.data = wifi_nrf_umac_patch_pri_bimg;
	fw_info.umac_patch_pri.size = sizeof(wifi_nrf_umac_patch_pri_bimg);
	fw_info.umac_patch_sec.data = wifi_nrf_umac_patch_sec_bin;
	fw_info.umac_patch_sec.size = sizeof(wifi_nrf_umac_patch_sec_bin);

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, true);
#endif /* CONFIG_NRF_WIFI */
	/* Load the FW patches to the RPU */
	status = wifi_nrf_fmac_fw_load(rpu_ctx,
				       &fw_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		printf("%s: wifi_nrf_fmac_fw_load failed\n", __func__);
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, false);
#endif /* CONFIG_NRF_WIFI */

	return status;
}

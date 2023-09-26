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

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_fw_info fw_info;
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH */

	memset(&fw_info, 0, sizeof(fw_info));
	fw_info.lmac_patch_pri.data = (void *) nrf_wifi_lmac_patch_pri_bimg;
	fw_info.lmac_patch_pri.size = sizeof(nrf_wifi_lmac_patch_pri_bimg);
	fw_info.lmac_patch_sec.data = (void *) nrf_wifi_lmac_patch_sec_bin;
	fw_info.lmac_patch_sec.size = sizeof(nrf_wifi_lmac_patch_sec_bin);
	fw_info.umac_patch_pri.data = (void *) nrf_wifi_umac_patch_pri_bimg;
	fw_info.umac_patch_pri.size = sizeof(nrf_wifi_umac_patch_pri_bimg);
	fw_info.umac_patch_sec.data = (void *) nrf_wifi_umac_patch_sec_bin;
	fw_info.umac_patch_sec.size = sizeof(nrf_wifi_umac_patch_sec_bin);

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, true);
#endif /* CONFIG_NRF_WIFI */
	/* Load the FW patches to the RPU */
	status = nrf_wifi_fmac_fw_load(rpu_ctx,
				       &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		printf("%s: nrf_wifi_fmac_fw_load failed\n", __func__);
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, false);
#endif /* CONFIG_NRF_WIFI */

	return status;
}

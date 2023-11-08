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
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
#include <zephyr/drivers/flash/nrf_qspi_nor.h>
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF700X_LOG_LEVEL);

#include <fmac_main.h>

/* INCBIN macro Taken from https://gist.github.com/mmozeiko/ed9655cf50341553d282 */
#define STR2(x) #x
#define STR(x) STR2(x)

#ifdef __APPLE__
#define USTR(x) "_" STR(x)
#else
#define USTR(x) STR(x)
#endif

#ifdef _WIN32
#define INCBIN_SECTION ".rdata, \"dr\""
#elif defined __APPLE__
#define INCBIN_SECTION "__TEXT,__const"
#else
#define INCBIN_SECTION ".rodata"
#endif

/* this aligns start address to 16 and terminates byte array with explicit 0
 * which is not really needed, feel free to change it to whatever you want/need
 */
#define INCBIN(prefix, name, file) \
	__asm__(".section " INCBIN_SECTION "\n" \
			".global " USTR(prefix) "_" STR(name) "_start\n" \
			".balign 16\n" \
			USTR(prefix) "_" STR(name) "_start:\n" \
			".incbin \"" file "\"\n" \
			\
			".global " STR(prefix) "_" STR(name) "_end\n" \
			".balign 1\n" \
			USTR(prefix) "_" STR(name) "_end:\n" \
			".byte 0\n" \
	); \
	extern __aligned(16)    const char prefix ## _ ## name ## _start[]; \
	extern                  const char prefix ## _ ## name ## _end[];

INCBIN(_bin, nrf70_fw, STR(CONFIG_NRF_WIFI_FW_BIN));

enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_fmac_fw_info fw_info = { 0 };
#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, nordic_qspi_nor));
#endif /* CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP */

	status = nrf_wifi_fmac_fw_parse(rpu_ctx, (const uint8_t *)_bin_nrf70_fw_start,
					_bin_nrf70_fw_end - _bin_nrf70_fw_start, &fw_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_parse failed", __func__);
		return status;
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, true);
#endif /* CONFIG_NRF_WIFI */
	/* Load the FW patches to the RPU */
	status = nrf_wifi_fmac_fw_load(rpu_ctx, &fw_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_fw_load failed", __func__);
	}

#if defined(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP) && defined(CONFIG_NORDIC_QSPI_NOR)
	nrf_qspi_nor_xip_enable(flash_dev, false);
#endif /* CONFIG_NRF_WIFI */

	return status;
}

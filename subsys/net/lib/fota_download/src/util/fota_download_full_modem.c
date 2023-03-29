/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/drivers/flash.h>
#include <dfu/dfu_target_full_modem.h>
#include <nrf_modem_bootloader.h>
#include <dfu/fmfu_fdev.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fota_download_full_modem, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t fota_download_fulmodem_buf[CONFIG_FOTA_DOWNLOAD_FULL_MODEM_BUF_SZ];
static const struct device *flash_dev = DEVICE_DT_GET_ANY(jedec_spi_nor);

int fota_download_full_modem_apply_update(void)
{
	int ret;

	LOG_INF("Applying full modem firmware update from external flash");

	ret = nrf_modem_lib_shutdown();
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_shutdown() failed: %d", ret);
		return ret;
	}

	ret = nrf_modem_lib_bootloader_init();
	if (ret != 0) {
		LOG_ERR("nrf_modem_lib_bootloader_init() failed: %d", ret);
		return ret;
	}

	ret = fmfu_fdev_load(fota_download_fulmodem_buf, sizeof(fota_download_fulmodem_buf),
			     flash_dev, 0);
	if (ret != 0) {
		LOG_ERR("fmfu_fdev_load failed: %d", ret);
		return ret;
	}
	LOG_INF("Modem firmware update completed");
	return 0;
}

int fota_download_full_modem_stream_params_init(void)
{
	int ret = 0;

	const struct dfu_target_full_modem_params params = {
		.buf = fota_download_fulmodem_buf,
		.len = sizeof(fota_download_fulmodem_buf),
		.dev = &(struct dfu_target_fmfu_fdev){ .dev = flash_dev, .offset = 0, .size = 0 }
	};

	ret = dfu_target_full_modem_cfg(&params);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("dfu_target_full_modem_cfg failed: %d", ret);
	} else {
		ret = 0;
	}

	return ret;
}

int fota_download_full_modem_pre_init(void)
{
	if (!flash_dev || !device_is_ready(flash_dev)) {
		LOG_ERR("Flash device not ready");
		return -EIO;
	}

	return fota_download_full_modem_stream_params_init();
}

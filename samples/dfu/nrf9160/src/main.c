/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/dfu/mcuboot.h>
#include <modem/nrf_modem_lib.h>
#include <net/fota_download.h>
#include <modem/lte_lc.h>
#include <dfu/dfu_target_uart.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

/**
 * @typedef update_start_cb
 *
 * @brief Signature for callback invoked when button is pressed.
 */
typedef void (*update_start_cb)(void);

/**
 * @brief Initialization struct.
 */

static struct k_work fota_work;
static update_start_cb update_start;
static char filename[128] = {0};

BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
	     "This sample does not support auto init and connect");

struct update_sample_init_params {
	/* Callback invoked when the button is pressed */
	update_start_cb update_start;

	/* Update file to download */
	const char *filename;
};

static struct dfu_target_uart_remote remotes[] = {
	{
		.identifier = 1,
		.device = DEVICE_DT_GET(DT_CHOSEN(sample_dfu_uart))
	}
};

static uint8_t out_buffer[CONFIG_DOWNLOAD_CLIENT_BUF_SIZE + 50] = {0};

static struct dfu_target_uart_params config = {
	.remote_count = ARRAY_SIZE(remotes),
	.remotes = remotes,
	.buffer = out_buffer,
	.buf_size = ARRAY_SIZE(out_buffer)
};

static void fota_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	if (update_start != NULL) {
		update_start();
	}
}

static void modem_configure(void)
{
	int err;

	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
}

static int update_sample_init(struct update_sample_init_params *params)
{
	if (params == NULL || params->update_start == NULL || params->filename == NULL) {
		return -EINVAL;
	}

	k_work_init(&fota_work, fota_work_cb);

	update_start = params->update_start;
	strcpy(filename, params->filename);

	modem_configure();

	return 0;
}

static void update_sample_done(void)
{
	lte_lc_deinit();
}

static void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		LOG_ERR("Received error from fota_download\n");
		break;

	case FOTA_DOWNLOAD_EVT_FINISHED:
		update_sample_done();
		break;

	case FOTA_DOWNLOAD_EVT_PROGRESS:
		LOG_DBG("Progress: %d %%\n", evt->progress);
		break;

	default:
		break;
	}
}

static void update_sample_start(void)
{
	int err;

	/* Functions for getting the host and file */
	err = fota_download_start(CONFIG_DOWNLOAD_HOST, filename, -1, 0, 0);
	if (err != 0) {
		LOG_ERR("fota_download_start() failed, err %d\n", err);
	}
}

void main(void)
{
	int err;

	LOG_DBG("HTTP application update sample started\n");

	err = nrf_modem_lib_init(NORMAL_MODE);
	if (err) {
		LOG_ERR("Failed to initialize modem library!");
		return;
	}

	dfu_target_uart_cfg(&config);

	err = fota_download_init(fota_dl_handler);
	if (err) {
		LOG_ERR("fota_download_init() failed, err %d\n", err);
		return;
	}

	err = update_sample_init(&(struct update_sample_init_params) {
		.update_start = update_sample_start,
		.filename = CONFIG_DOWNLOAD_FILE
	});
	if (err) {
		LOG_ERR("update_sample_init() failed, err %d\n", err);
		return;
	}

	k_work_submit(&fota_work);
}

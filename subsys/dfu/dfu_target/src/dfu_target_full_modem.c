/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include <dfu/dfu_target_full_modem.h>

LOG_MODULE_REGISTER(dfu_target_full_modem, CONFIG_DFU_TARGET_LOG_LEVEL);

#define FULL_MODEM_HEADER_MAGIC 0x84d2

static bool configured;

bool dfu_target_full_modem_identify(const void *const buf)
{
	return *((const uint16_t *)buf) == FULL_MODEM_HEADER_MAGIC;
}

int dfu_target_full_modem_cfg(const struct dfu_target_full_modem_params *params)
{
	int err;

	err = dfu_target_stream_init(
		&(struct dfu_target_stream_init){ .id = "DFU_FULL_MODEM",
						  .buf = params->buf,
						  .len = params->len,
						  .fdev = params->dev->dev,
						  .offset = params->dev->offset,
						  .size = params->dev->size,
						  .cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	configured = true;

	return 0;
}

int dfu_target_full_modem_init(size_t file_size,
			       dfu_target_callback_t callback)
{
	if (!configured) {
		return -EPERM;
	}

	return 0;
}

int dfu_target_full_modem_offset_get(size_t *out)
{
	return dfu_target_stream_offset_get(out);
}

int dfu_target_full_modem_write(const void *const buf, size_t len)
{
	return dfu_target_stream_write(buf, len);
}

int dfu_target_full_modem_done(bool successful)
{
	configured = false;

	if (successful) {
		LOG_INF("Modem firmware downloaded to flash device");
	}

	return dfu_target_stream_done(successful);
}

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>
#include <dfu/dfu_target_full_modem.h>
#include <zephyr/storage/flash_map.h>
#include <flash_map_pm.h>

LOG_MODULE_REGISTER(dfu_target_full_modem, CONFIG_DFU_TARGET_LOG_LEVEL);

#define FULL_MODEM_HEADER_MAGIC 0x84d2

static bool configured;
static struct dfu_target_full_modem_params dfu_params;
static struct dfu_target_fmfu_fdev flash_dev;

bool dfu_target_full_modem_identify(const void *const buf)
{
	return *((const uint16_t *)buf) == FULL_MODEM_HEADER_MAGIC;
}

int dfu_target_full_modem_cfg(const struct dfu_target_full_modem_params *params)
{
	if (configured) {
		return -EALREADY;
	}

	if (params->buf == NULL ||
	   ((params->dev->dev == NULL) &&
	    (!IS_ENABLED(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION)))) {
		return -EINVAL;
	}

	/* Clone parameters */
	dfu_params.buf = params->buf;
	dfu_params.len = params->len;
	dfu_params.dev = &flash_dev;

#if defined(CONFIG_DFU_TARGET_FULL_MODEM_USE_EXT_PARTITION)
	const struct flash_area *fa;
	int err;

	err = flash_area_open(FLASH_AREA_ID(fmfu_storage), &fa);
	if (err) {
		return -ENODEV;
	}

	flash_dev.dev = fa->fa_dev;
	flash_dev.offset = fa->fa_off;
	flash_dev.size = fa->fa_size;
#else
	flash_dev = *params->dev;
#endif

	configured = true;

	return 0;
}

int dfu_target_full_modem_fdev_get(struct dfu_target_fmfu_fdev * const fdev)
{
	if (!configured) {
		return -EPERM;
	}

	if (!fdev) {
		return -EINVAL;
	}

	*fdev = flash_dev;

	return 0;
}

int dfu_target_full_modem_init(size_t file_size, int img_num, dfu_target_callback_t callback)
{
	int err;

	if (!configured) {
		return -EPERM;
	}

	/* Init stream by parameters */
	err = dfu_target_stream_init(
		&(struct dfu_target_stream_init){ .id = "DFU_FULL_MODEM",
						  .buf = dfu_params.buf,
						  .len = dfu_params.len,
						  .fdev = dfu_params.dev->dev,
						  .offset = dfu_params.dev->offset,
						  .size = dfu_params.dev->size,
						  .cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
	}

	return err;
}

int dfu_target_full_modem_offset_get(size_t *out)
{
	if (!configured) {
		return -EPERM;
	}

	return dfu_target_stream_offset_get(out);
}

int dfu_target_full_modem_write(const void *const buf, size_t len)
{
	if (!configured) {
		return -EPERM;
	}

	return dfu_target_stream_write(buf, len);
}

int dfu_target_full_modem_done(bool successful)
{
	if (!configured) {
		return -EPERM;
	}

	if (successful) {
		LOG_INF("Modem firmware downloaded to flash device");
	}

	return dfu_target_stream_done(successful);
}

int dfu_target_full_modem_schedule_update(int img_num)
{
	ARG_UNUSED(img_num);

	if (!configured) {
		return -EPERM;
	}

	return 0;
}

int dfu_target_full_modem_reset(void)
{
	if (!configured) {
		return -EPERM;
	}

	return dfu_target_stream_reset();
}

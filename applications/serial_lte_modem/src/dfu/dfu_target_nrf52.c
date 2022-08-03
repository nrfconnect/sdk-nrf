/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Ensure 'strnlen' is available even with -std=c99. If
 * _POSIX_C_SOURCE was defined we will get a warning when referencing
 * 'strnlen'. If this proves to cause trouble we could easily
 * re-implement strnlen instead, perhaps with a different name, as it
 * is such a simple function.
 */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <pm_config.h>
#include <zephyr/logging/log.h>
#include <nrfx.h>
#include <zephyr/dfu/mcuboot.h>
#include <dfu/dfu_target.h>
#include <dfu/dfu_target_stream.h>

LOG_MODULE_REGISTER(target_dfu, CONFIG_SLM_LOG_LEVEL);

#define MAX_FILE_SEARCH_LEN 500
#define MCUBOOT_SECONDARY_LAST_PAGE_ADDR                                       \
	(PM_MCUBOOT_SECONDARY_ADDRESS + PM_MCUBOOT_SECONDARY_SIZE - 1)

#define IS_ALIGNED_32(POINTER) (((uintptr_t)(const void *)(POINTER)) % 4 == 0)

static uint8_t *stream_buf;
static size_t stream_buf_len;
static size_t stream_buf_bytes;

int dfu_target_nrf52_set_buf(uint8_t *buf, size_t len)
{
	if (buf == NULL) {
		return -EINVAL;
	}

	if (!IS_ALIGNED_32(buf)) {
		return -EINVAL;
	}

	stream_buf = buf;
	stream_buf_len = len;

	return 0;
}

int dfu_target_nrf52_init(size_t file_size, dfu_target_callback_t cb)
{
	ARG_UNUSED(cb);
	const struct device *flash_dev;
	int err;

	stream_buf_bytes = 0;

	if (stream_buf == NULL) {
		LOG_ERR("Missing stream_buf, call '..set_buf' before '..init");
		return -ENODEV;
	}

	if (file_size > PM_MCUBOOT_SECONDARY_SIZE) {
		LOG_ERR("Requested file too big to fit in flash %zu > 0x%x",
			file_size, PM_MCUBOOT_SECONDARY_SIZE);
		return -EFBIG;
	}

	flash_dev = DEVICE_GET_DT(DT_NODELABEL(PM_MCUBOOT_SECONDARY_DEV));

	err = dfu_target_stream_init(&(struct dfu_target_stream_init){
		.id = "NRF52",
		.fdev = flash_dev,
		.buf = stream_buf,
		.len = stream_buf_len,
		.offset = PM_MCUBOOT_SECONDARY_ADDRESS,
		.size = PM_MCUBOOT_SECONDARY_SIZE,
		.cb = NULL });
	if (err < 0) {
		LOG_ERR("dfu_target_stream_init failed %d", err);
		return err;
	}

	return 0;
}

int dfu_target_nrf52_offset_get(size_t *out)
{
	int err = 0;

	err = dfu_target_stream_offset_get(out);
	if (err == 0) {
		*out += stream_buf_bytes;
	}

	return err;
}

int dfu_target_nrf52_write(const void *const buf, size_t len)
{
	stream_buf_bytes = (stream_buf_bytes + len) % stream_buf_len;

	return dfu_target_stream_write(buf, len);
}

int dfu_target_nrf52_done(bool successful)
{
	int err = 0;

	err = dfu_target_stream_done(successful);
	if (err != 0) {
		LOG_ERR("dfu_target_stream_done error %d", err);
		return err;
	}

	if (successful) {
		LOG_INF("DFU image upgrade scheduled");
	} else {
		LOG_ERR("DFU image upgrade aborted.");
	}

	return err;
}

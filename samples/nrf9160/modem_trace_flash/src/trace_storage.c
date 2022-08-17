/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>

#include "trace_storage.h"

LOG_MODULE_REGISTER(modem_trace_storage, CONFIG_MODEM_TRACE_STORAGE_LOG_LEVEL);

#define EXT_FLASH_DEVICE DEVICE_DT_GET(DT_ALIAS(ext_flash))
#define TRACE_OFFSET FLASH_AREA_OFFSET(modem_trace)
#define TRACE_SIZE FLASH_AREA_SIZE(modem_trace)
#define BUF_LEN 1024

static const struct flash_area *modem_trace_area;
static const struct device *flash_dev;
static struct stream_flash_ctx stream;
static uint8_t stream_buf[BUF_LEN];
static bool has_flushed;

/* Persist trace size in .noinit RAM to survive warm boot. This functionality is not used in the
 * sample now, but is here to show how it's possible to know the size written to flash after a
 * crash/restart.
 */
static __noinit uint32_t magic;
static __noinit uint32_t traces_size;

static int64_t tot_write_time;

static int internal_flash_write(const void *data, size_t len, bool flush)
{
	int64_t uptime_ref = k_uptime_get();
	int64_t write_time;

	int err = stream_flash_buffered_write(&stream, data, len, flush);

	if (err != 0) {
		LOG_ERR("stream_flash_buffered_write error %d", err);
		return err;
	}

	write_time = k_uptime_delta(&uptime_ref);
	tot_write_time += write_time;

	traces_size += len;

	return 0;
}

static const uint32_t magic_val = 0x152ac523;
int trace_storage_init(void)
{
	int err;

	LOG_DBG("trace_storage_init\n");

	/* After a cold boot the magic and traces_size will contain random values. */
	if (magic != magic_val) {
		/* This is a cold reboot. Initialize. */
		LOG_DBG("Initializing .noinit\n");
		magic = magic_val;
		traces_size = 0;
	} else if (traces_size > 0) {
		LOG_DBG("Found modem traces stored in flash after soft reboot: %d", traces_size);
		/* Add custom logic here to preserve traces before erasing the flash */
	}

	traces_size = 0;

	err = flash_area_open(FLASH_AREA_ID(modem_trace), &modem_trace_area);
	if (err) {
		LOG_ERR("flash_area_open error:  %d", err);
		return -ENODEV;
	}

	err = flash_area_has_driver(modem_trace_area);
	if (err != 1) {
		LOG_ERR("flash_area_has_driver error: %d\n", err);
	}

	flash_dev = flash_area_get_device(modem_trace_area);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get flash device\n");
		return -ENODEV;
	}

	err = stream_flash_init(&stream, flash_dev, stream_buf, BUF_LEN, TRACE_OFFSET, TRACE_SIZE,
		NULL);
	if (err) {
		LOG_ERR("stream_flash_init error: %d", err);
		return err;
	}

	int64_t erase_before = k_uptime_get();

	/* In order to write modem traces uninterrupted at full speed, we need to erase the flash
	 * before starting the modem tracing.
	 */
	err = flash_erase(flash_dev, TRACE_OFFSET, TRACE_SIZE);
	if (err) {
		LOG_ERR("flash_erase error: %d", err);
		return err;
	}
	int64_t erase_time = k_uptime_delta(&erase_before);

	LOG_DBG("Erased %lu bytes in %lld ms", (unsigned long) TRACE_SIZE, erase_time);
	LOG_DBG("Erase speed %lld kb/s ", (TRACE_SIZE * 8 / erase_time));

	tot_write_time = 0;

	LOG_DBG("Modem trace flash storage initialized\n");

	return 0;
}

int trace_storage_flush(void)
{
	int err = internal_flash_write(NULL, 0, true);

	if (err != 0) {
		LOG_ERR("Flush to flash failed: %d", err);
		return err;
	}

	has_flushed = true;

	LOG_DBG("Written %d bytes in %lld ms", traces_size, tot_write_time);
	LOG_DBG("Throughput = %lld kb/s ", (traces_size * 8 / tot_write_time));

	return 0;
}

int trace_storage_read(uint8_t *buf, size_t len, size_t offset)
{
	__ASSERT(traces_size > offset, "offset should not exceed size of traces");
	uint32_t bytes_left = traces_size - offset;
	size_t read_len = MIN(bytes_left, len);

	if (!has_flushed) {
		return -EIO;
	}

	int err = flash_area_read(modem_trace_area, offset, buf, read_len);

	if (err != 0) {
		LOG_ERR("flash_area_read error %d", err);
		return err;
	}

	return read_len;
}

int trace_storage_write(const void *buf, size_t len)
{
	if (has_flushed) {
		return -EIO;
	}

	return internal_flash_write(buf, len, false);
}

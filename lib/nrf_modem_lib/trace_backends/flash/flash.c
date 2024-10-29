/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <modem/trace_backend.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

#define EXT_FLASH_DEVICE DEVICE_DT_GET(DT_ALIAS(ext_flash))
#define TRACE_OFFSET FLASH_AREA_OFFSET(modem_trace)
#define TRACE_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE

#define BUF_SIZE CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE

#define TRACE_MAGIC_INITIALIZED 0x152ac523

static trace_backend_processed_cb trace_processed_callback;

static const struct flash_area *modem_trace_area;
static const struct device *flash_dev;
static struct flash_sector trace_flash_sectors[CONFIG_NRF_MODEM_LIB_TRACE_FLASH_SECTORS];

static struct fcb trace_fcb = {
	.f_flags = FCB_FLAGS_CRC_DISABLED,
};

/* Flash backend needs to wait for a sector to be cleared and will give the semaphore instead.
 * This should be cleaned up with a separate API, but we declare the semaphore as extern for now.
 */
extern struct k_sem trace_clear_sem;

/* Store in __noinit RAM to perserve in warm boot. */
static __noinit uint32_t magic;
static __noinit size_t read_offset;
static __noinit struct fcb_entry loc;
static __noinit struct flash_sector *sector;

static __noinit size_t trace_bytes_unread;
static __noinit size_t flash_buf_written;
static __noinit uint8_t flash_buf[BUF_SIZE];

static bool is_initialized;

static struct k_sem fcb_sem;

static size_t buffer_append(const void *data, size_t len)
{
	size_t append_len;

	append_len = MIN(len, sizeof(flash_buf) - flash_buf_written);

	memcpy(&flash_buf[flash_buf_written], data, append_len);

	flash_buf_written += append_len;
	trace_bytes_unread += append_len;

	return append_len;
}

static int fcb_walk_callback(struct fcb_entry_ctx *loc_ctx, void *arg)
{
	if ((loc_ctx->loc.fe_sector == sector) && (loc_ctx->loc.fe_elem_off < loc.fe_elem_off)) {
		return 0;
	}

	trace_bytes_unread -= loc_ctx->loc.fe_data_len;
	return 0;
}

static int buffer_flush_to_flash(void)
{
	int err;
	struct fcb_entry loc_flush;

	if (!is_initialized) {
		return -EPERM;
	}

	if (!flash_buf_written) {
		return -ENODATA;
	}

	k_sem_take(&fcb_sem, K_FOREVER);

	err = fcb_append(&trace_fcb, flash_buf_written, &loc_flush);
	if (err) {
		if (IS_ENABLED(CONFIG_NRF_MODEM_TRACE_FLASH_NOSPACE_ERASE_OLDEST)) {
			/* Find the number of trace bytes in oldest sector (that is not read). */

			/* Get first sector in FCB */
			loc_flush.fe_sector = 0;
			loc_flush.fe_elem_off = 0;
			err = fcb_getnext(&trace_fcb, &loc_flush);

			/* Walk sector to remove unread trace data from count. */
			err = fcb_walk(&trace_fcb, loc_flush.fe_sector, fcb_walk_callback, NULL);
			if (err) {
				LOG_ERR("fcb_walk failed, err %d", err);
				goto out;
			}

			/* Erase the oldest sector and append again. */
			err = fcb_rotate(&trace_fcb);
			if (err) {
				LOG_ERR("fcb_rotate failed, err %d", err);
				goto out;
			}
			err = fcb_append(&trace_fcb, flash_buf_written, &loc_flush);
		}

		if (err) {
			if (err != -ENOSPC) {
				LOG_ERR("fcb_append failed, err %d", err);
			}

			goto out;
		}
	}

	err = flash_area_write(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc_flush), flash_buf, flash_buf_written);
	if (err) {
		LOG_ERR("flash_area_write failed, err %d", err);
		goto out;
	}

	err = fcb_append_finish(&trace_fcb, &loc_flush);
	if (err) {
		LOG_ERR("fcb_append_finish failed, err %d", err);
		goto out;
	}

	flash_buf_written = 0;

out:
	k_sem_give(&fcb_sem);
	return err;
}

static int trace_flash_erase(void)
{
	int err;

	LOG_INF("Erasing external flash");

	err = flash_area_erase(modem_trace_area, 0, TRACE_SIZE);
	if (err) {
		LOG_ERR("flash_area_erase error: %d", err);
	}

	return err;
}

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;
	const struct flash_parameters *fparam;

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	k_sem_init(&fcb_sem, 0, 1);

	trace_processed_callback = trace_processed_cb;

	err = flash_area_open(FIXED_PARTITION_ID(MODEM_TRACE), &modem_trace_area);
	if (err) {
		LOG_ERR("flash_area_open error:  %d", err);
		return -ENODEV;
	}

	err = flash_area_has_driver(modem_trace_area);
	if (err == -ENODEV) {
		LOG_ERR("flash_area_has_driver error: %d\n", err);
		return -ENODEV;
	}

	flash_dev = flash_area_get_device(modem_trace_area);
	if (flash_dev == NULL) {
		LOG_ERR("Failed to get flash device\n");
		return -ENODEV;
	}

	/* After a cold boot the magic will contain random values. */
	if (magic != TRACE_MAGIC_INITIALIZED) {
		LOG_DBG("Trace magic not found, initializing");
		read_offset = 0;
		trace_bytes_unread = 0;
		flash_buf_written = 0;
		memset(&loc, 0, sizeof(loc));
		sector = NULL;
		magic = TRACE_MAGIC_INITIALIZED;
		trace_flash_erase();
	} else {
		LOG_DBG("Trace magic found, skipping initialization");
	}

	uint32_t f_sector_cnt = sizeof(trace_flash_sectors) / sizeof(struct flash_sector);

	err = flash_area_get_sectors(
		FIXED_PARTITION_ID(MODEM_TRACE), &f_sector_cnt, trace_flash_sectors);
	if (err) {
		LOG_ERR("flash_area_get_sectors error: %d", err);
		return err;
	}

	fparam = flash_get_parameters(flash_dev);

	trace_fcb.f_magic = TRACE_MAGIC_INITIALIZED;
	trace_fcb.f_erase_value = fparam->erase_value;
	trace_fcb.f_sector_cnt = f_sector_cnt;
	trace_fcb.f_sectors = trace_flash_sectors;

	LOG_DBG("Sectors: %d, first sector: %p, sector size: %d",
		f_sector_cnt, trace_flash_sectors, trace_flash_sectors[0].fs_size);

	err = fcb_init(FIXED_PARTITION_ID(MODEM_TRACE), &trace_fcb);
	if (err) {
		LOG_ERR("fcb_init error: %d", err);
		return err;
	}

	is_initialized = true;

	LOG_DBG("Modem trace flash storage initialized\n");

	k_sem_give(&fcb_sem);
	return 0;
}

size_t trace_backend_data_size(void)
{
	return trace_bytes_unread;
}

/* Read from offset
 * FCB sem has to be taken before calling this function!
 */
static int read_from_offset(void *buf, size_t len)
{
	int err;
	size_t to_read;

	to_read = MIN(len, loc.fe_data_len - read_offset);
	err = flash_area_read(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc) + read_offset, buf, to_read);
	if (err) {
		LOG_ERR("Flash_area_read failed, err %d", err);
		return err;
	}

	trace_bytes_unread -= to_read;

	read_offset += to_read;
	if (read_offset >= loc.fe_data_len) {
		read_offset = 0;
	}

	return to_read;
}

int trace_backend_read(void *buf, size_t len)
{
	int err;
	size_t ret;
	size_t to_read = 0;

	if (!is_initialized) {
		return -EPERM;
	}

	if (!buf) {
		return -EINVAL;
	}

	k_sem_take(&fcb_sem, K_FOREVER);

	if (read_offset != 0 && loc.fe_sector) {
		err = read_from_offset(buf, len);
		goto out;
	}

	err = fcb_getnext(&trace_fcb, &loc);
	if (err == -ENOTSUP && !flash_buf_written) {
		/* Nothing to read */
		loc.fe_sector = 0;
		loc.fe_elem_off = 0;
		read_offset = 0;
		sector = NULL;
		err = -ENODATA;
		goto out;
	} else if (err == -ENOTSUP && flash_buf_written) {
		to_read = MIN(flash_buf_written, len);
		memcpy(buf, flash_buf, to_read);
		if (to_read != flash_buf_written) {
			/* We haven't read all, move the rest to start of buffer */
			memmove(flash_buf, &flash_buf[to_read],
				flash_buf_written - to_read);
		}

		flash_buf_written -= to_read;
		trace_bytes_unread -= to_read;

		err = to_read;
		goto out;

	} else if (err) {
		goto out;
	}

	err = read_from_offset(buf, len);

out:
	ret = err;

	/* Erase if done with previous sector. */
	if (sector && (sector != loc.fe_sector)) {
		err = fcb_rotate(&trace_fcb);
		if (err) {
			LOG_ERR("Failed to erase read sector, err %d", err);
			k_sem_give(&fcb_sem);
			/* Return what we have read */
			return ret;
		}

		k_sem_give(&trace_clear_sem);
	}

	sector = loc.fe_sector;

	k_sem_give(&fcb_sem);
	return ret;
}

static int stream_write(const void *buf, size_t len)
{
	int ret;
	int written;
	size_t written_total = 0;
	size_t bytes_left = len;
	const uint8_t *bytes = buf;

	if (!is_initialized) {
		return -EPERM;
	}

	while (bytes_left) {
		written = buffer_append(&bytes[len - bytes_left], bytes_left);
		written_total += written;

		if (flash_buf_written >= sizeof(flash_buf)) {
			ret = buffer_flush_to_flash();
			if (ret) {
				LOG_ERR("buffer_flush_to_flash error %d", ret);
				if (written_total) {
					ret = trace_processed_callback(written);
					if (ret < 0) {
						LOG_ERR("trace_processed_callback failed: %d", ret);
						return ret;
					}
					return written_total;
				}
				return ret;
			}
		}
		if (written > 0) {
			bytes_left -= written;
			ret = trace_processed_callback(written);
			if (ret < 0) {
				LOG_ERR("trace_processed_callback 2 failed: %d", ret);
				return ret;
			}
		}
	}

	return written_total;
}

int trace_backend_write(const void *data, size_t len)
{
	int write_ret = stream_write(data, len);

	if (write_ret < 0) {
		if (write_ret != -ENOSPC) {
			LOG_ERR("write failed: %d", write_ret);
		}
	}

	return write_ret;
}

int trace_backend_clear(void)
{
	int err;

	if (!is_initialized) {
		return -EPERM;
	}

	k_sem_take(&fcb_sem, K_FOREVER);
	LOG_DBG("Clearing trace storage");
	flash_buf_written = 0;
	err = fcb_clear(&trace_fcb);

	loc.fe_sector = 0;
	loc.fe_elem_off = 0;
	trace_bytes_unread = 0;
	read_offset = 0;
	sector = NULL;

	k_sem_give(&fcb_sem);

	return err;
}

int trace_backend_deinit(void)
{
	buffer_flush_to_flash();
	return 0;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.data_size = trace_backend_data_size,
	.read = trace_backend_read,
	.clear = trace_backend_clear,
};

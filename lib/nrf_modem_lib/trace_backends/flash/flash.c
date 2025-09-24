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

/* Partition offset is implicit in flash_area */

#if USE_PARTITION_MANAGER
#define MODEM_TRACE_PARTITION	MODEM_TRACE
#else
#define MODEM_TRACE_PARTITION	modem_trace
#endif

#define BUF_SIZE		CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_BUF_SIZE
#define TRACE_MAGIC_INITIALIZED 0x152ac523
#define PEEK_AT_OFFSET_MAGIC	0x153ac522

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

struct flash_backend_state {
	uint32_t magic;
	size_t read_offset;
	struct fcb_entry loc;
	struct flash_sector *sector;
	size_t trace_bytes_unread;
	size_t flash_buf_written;
	uint8_t flash_buf[BUF_SIZE];
};

struct peek_at_cache {
	size_t offset;
	uint32_t magic;
	struct fcb_entry entry;
	size_t in_entry_offset;
};

/* Store in __noinit RAM to perserve in warm boot. */
static __noinit struct flash_backend_state backend_state;

static bool is_initialized;
static struct k_sem fcb_sem;
static struct peek_at_cache peek_at_cache;

static inline void peek_at_cache_set(size_t offset, struct fcb_entry *entry, size_t in_entry_offset)
{
	peek_at_cache.magic = PEEK_AT_OFFSET_MAGIC;
	peek_at_cache.offset = offset;
	peek_at_cache.in_entry_offset = in_entry_offset;

	memcpy(&peek_at_cache.entry, entry, sizeof(peek_at_cache.entry));
}

static inline void peek_at_cache_invalidate(void)
{
	memset(&peek_at_cache, 0, sizeof(peek_at_cache));
}

static inline bool peek_at_cache_is_valid(void)
{
	bool magic_valid = peek_at_cache.magic == PEEK_AT_OFFSET_MAGIC;
	bool entry_valid = peek_at_cache.entry.fe_sector != NULL;

	return magic_valid && entry_valid;
}

static size_t buffer_append(const void *data, size_t len)
{
	size_t append_len;

	append_len = MIN(len, sizeof(backend_state.flash_buf) - backend_state.flash_buf_written);

	memcpy(&backend_state.flash_buf[backend_state.flash_buf_written], data, append_len);

	backend_state.flash_buf_written += append_len;
	backend_state.trace_bytes_unread += append_len;

	return append_len;
}

static int fcb_walk_callback(struct fcb_entry_ctx *loc_ctx, void *arg)
{
	if ((loc_ctx->loc.fe_sector == backend_state.sector) &&
	    (loc_ctx->loc.fe_elem_off < backend_state.loc.fe_elem_off)) {
		return 0;
	}

	backend_state.trace_bytes_unread -= loc_ctx->loc.fe_data_len;

	return 0;
}

static int buffer_flush_to_flash(void)
{
	int err;
	struct fcb_entry loc_flush;

	if (!is_initialized) {
		return -EPERM;
	}

	if (!backend_state.flash_buf_written) {
		return -ENODATA;
	}

	k_sem_take(&fcb_sem, K_FOREVER);

	err = fcb_append(&trace_fcb, backend_state.flash_buf_written, &loc_flush);
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

			err = fcb_append(&trace_fcb, backend_state.flash_buf_written, &loc_flush);

			peek_at_cache_invalidate();
		}

		if (err) {
			if (err != -ENOSPC) {
				LOG_ERR("fcb_append failed, err %d", err);
			}

			goto out;
		}
	}

	err = flash_area_write(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc_flush), backend_state.flash_buf,
		backend_state.flash_buf_written);
	if (err) {
		LOG_ERR("flash_area_write failed, err %d", err);

		goto out;
	}

	err = fcb_append_finish(&trace_fcb, &loc_flush);
	if (err) {
		LOG_ERR("fcb_append_finish failed, err %d", err);

		goto out;
	}

	backend_state.flash_buf_written = 0;

out:
	k_sem_give(&fcb_sem);

	return err;
}

static int trace_flash_erase(void)
{
	int err;

	LOG_INF("Erasing external flash");

	err = flash_area_erase(modem_trace_area, 0, modem_trace_area->fa_size);
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

	err = flash_area_open(FIXED_PARTITION_ID(MODEM_TRACE_PARTITION), &modem_trace_area);
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
	if (backend_state.magic != TRACE_MAGIC_INITIALIZED) {
		LOG_DBG("Trace magic not found, initializing");

		backend_state.read_offset = 0;
		backend_state.trace_bytes_unread = 0;
		backend_state.flash_buf_written = 0;
		backend_state.sector = NULL;
		backend_state.magic = TRACE_MAGIC_INITIALIZED;

		memset(&backend_state.loc, 0, sizeof(backend_state.loc));
		trace_flash_erase();
	} else {
		LOG_DBG("Trace magic found, skipping initialization");
	}

	uint32_t f_sector_cnt = sizeof(trace_flash_sectors) / sizeof(struct flash_sector);

	err = flash_area_get_sectors(
		FIXED_PARTITION_ID(MODEM_TRACE_PARTITION), &f_sector_cnt, trace_flash_sectors);
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

	err = fcb_init(FIXED_PARTITION_ID(MODEM_TRACE_PARTITION), &trace_fcb);
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
	/* Ensure we never report more data than the partition can hold */
	return MIN(backend_state.trace_bytes_unread, modem_trace_area->fa_size);
}

/* Read from offset
 * FCB sem has to be taken before calling this function!
 */
static int read_from_offset(void *buf, size_t len)
{
	int err;
	size_t to_read;

	to_read = MIN(len, backend_state.loc.fe_data_len - backend_state.read_offset);

	err = flash_area_read(
		trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(backend_state.loc) +
		backend_state.read_offset, buf, to_read);
	if (err) {
		LOG_ERR("Flash_area_read failed, err %d", err);
		return err;
	}

	backend_state.trace_bytes_unread -= to_read;

	backend_state.read_offset += to_read;
	if (backend_state.read_offset >= backend_state.loc.fe_data_len) {
		backend_state.read_offset = 0;
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

	if (backend_state.read_offset != 0 && backend_state.loc.fe_sector) {
		err = read_from_offset(buf, len);

		goto out;
	}

	err = fcb_getnext(&trace_fcb, &backend_state.loc);
	if (err == -ENOTSUP && !backend_state.flash_buf_written) {
		/* Nothing to read */
		backend_state.loc.fe_sector = 0;
		backend_state.loc.fe_elem_off = 0;
		backend_state.read_offset = 0;
		backend_state.sector = NULL;

		err = -ENODATA;

		goto out;
	} else if (err == -ENOTSUP && backend_state.flash_buf_written) {
		to_read = MIN(backend_state.flash_buf_written, len);

		memcpy(buf, backend_state.flash_buf, to_read);

		if (to_read != backend_state.flash_buf_written) {
			/* We haven't read all, move the rest to start of buffer */
			memmove(backend_state.flash_buf, &backend_state.flash_buf[to_read],
				backend_state.flash_buf_written - to_read);
		}

		backend_state.flash_buf_written -= to_read;
		backend_state.trace_bytes_unread -= to_read;

		err = to_read;

		goto out;

	} else if (err) {
		goto out;
	}

	err = read_from_offset(buf, len);

out:
	ret = err;

	/* Erase if done with previous sector. */
	if (backend_state.sector && (backend_state.sector != backend_state.loc.fe_sector)) {
		err = fcb_rotate(&trace_fcb);
		if (err) {
			LOG_ERR("Failed to erase read sector, err %d", err);
			k_sem_give(&fcb_sem);
			/* Return what we have read */
			return ret;
		}

		peek_at_cache_invalidate();
		k_sem_give(&trace_clear_sem);
	}

	backend_state.sector = backend_state.loc.fe_sector;

	k_sem_give(&fcb_sem);

	return ret;
}

int trace_backend_peek_at(size_t offset, void *buf, size_t len)
{
	int err = 0;
	size_t copied = 0;
	struct fcb_entry entry;
	size_t read_offset = offset;
	size_t skip;
	/* Variables for caching the starting position when the offset is found within flash */
	struct fcb_entry start_entry = { 0 };
	size_t start_in_entry_offset = 0;
	bool have_start = false;

	if (!is_initialized) {
		return -EPERM;
	}

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	(void)k_sem_take(&fcb_sem, K_FOREVER);

	/* Fail early if requested offset is beyond available. */
	if (read_offset >= backend_state.trace_bytes_unread) {
		k_sem_give(&fcb_sem);

		return -EFAULT;
	}

	/* Initialize iterator from cache if possible, else from the oldest. */
	if (peek_at_cache_is_valid() && (read_offset >= peek_at_cache.offset)) {
		/* Start from cached entry and skip only the offset delta. */
		memcpy(&entry, &peek_at_cache.entry, sizeof(entry));
		err = 0;

		skip = (read_offset - peek_at_cache.offset) + peek_at_cache.in_entry_offset;
	} else {
		if (peek_at_cache_is_valid() && (read_offset < peek_at_cache.offset)) {
			peek_at_cache_invalidate();
		}

		entry.fe_sector = NULL;
		entry.fe_elem_off = 0;
		skip = read_offset;
		err = fcb_getnext(&trace_fcb, &entry);
	}

	while (err == 0) {
		size_t entry_len = entry.fe_data_len;
		size_t size_available;
		size_t size_to_read;

		/* If we need to skip, skip entire entries first. */
		if (skip >= entry_len) {
			skip -= entry_len;
			err = fcb_getnext(&trace_fcb, &entry);

			continue;
		}

		/* Ensure cache reflects the requested position inside current entry. */
		if (!have_start) {
			start_entry = entry;
			start_in_entry_offset = skip;
			have_start = true;
		}

		/* Copy from within this entry starting at in-entry offset. */
		size_available = entry_len - skip;
		size_to_read = MIN(size_available, len - copied);

		err = flash_area_read(trace_fcb.fap, FCB_ENTRY_FA_DATA_OFF(entry) + skip,
				   (uint8_t *)buf + copied, size_to_read);
		if (err) {
			LOG_ERR("flash_area_read (peek_at) failed, err %d", err);
			k_sem_give(&fcb_sem);

			return err;
		}

		/* We've consumed the skip within this entry. */
		skip = 0;

		copied += size_to_read;
		if (copied >= len) {
			k_sem_give(&fcb_sem);

			/* Cache the exact requested starting position for repeat reads */
			peek_at_cache_set(offset, &start_entry, start_in_entry_offset);

			return (int)copied;
		}

		/* Next entry */
		err = fcb_getnext(&trace_fcb, &entry);
	}

	/* After exhausting FCB, continue into RAM buffer if present. */
	if (backend_state.flash_buf_written > 0 && (backend_state.flash_buf_written > skip)) {
		size_t size_available = backend_state.flash_buf_written - skip;
		size_t size_to_read = MIN(size_available, len - copied);

		memcpy((uint8_t *)buf + copied, &backend_state.flash_buf[skip], size_to_read);

		copied += size_to_read;
	}

	k_sem_give(&fcb_sem);

	if (copied == 0) {
		return -ENODATA;
	}

	/* Set cache to requested starting position if we copied anything.
	 * In case of RAM tail, we don't cache the entry as it will be invalidated when the RAM tail
	 * is flushed.
	 */
	if (backend_state.flash_buf_written == 0) {
		peek_at_cache_set(offset, &start_entry, start_in_entry_offset);
	} else {
		peek_at_cache_invalidate();
	}

	return (int)copied;
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

		if (backend_state.flash_buf_written >= sizeof(backend_state.flash_buf)) {
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

	err = fcb_clear(&trace_fcb);

	backend_state.flash_buf_written = 0;
	backend_state.loc.fe_sector = 0;
	backend_state.loc.fe_elem_off = 0;
	backend_state.trace_bytes_unread = 0;
	backend_state.read_offset = 0;
	backend_state.sector = NULL;

	/* Storage rotated, invalidate cached peek_at iterator. */
	peek_at_cache_invalidate();

	k_sem_give(&fcb_sem);

	return err;
}

int trace_backend_deinit(void)
{
	buffer_flush_to_flash();
	peek_at_cache_invalidate();

	is_initialized = false;

	return 0;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.data_size = trace_backend_data_size,
	.read = trace_backend_read,
	.peek_at = trace_backend_peek_at,
	.clear = trace_backend_clear,
};

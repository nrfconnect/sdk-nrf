/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "log_backend_rpc_history.h"

#include <zephyr/fs/fcb.h>
#include <zephyr/sys/util.h>

#define LOG_HISTORY_MAGIC 0x7d2ac863
#define LOG_HISTORY_AREA FIXED_PARTITION_ID(log_history)

static struct fcb fcb;
static struct flash_sector fcb_sectors[CONFIG_LOG_BACKEND_RPC_HISTORY_STORAGE_FCB_NUM_SECTORS];
static struct fcb_entry last_popped;
static bool erase_oldest;
static K_MUTEX_DEFINE(fcb_lock);

void log_rpc_history_init(void)
{
	int rc;
	uint32_t sector_cnt = ARRAY_SIZE(fcb_sectors);

	rc = flash_area_get_sectors(LOG_HISTORY_AREA, &sector_cnt, fcb_sectors);

	if (rc) {
		goto out;
	}

	fcb.f_magic = LOG_HISTORY_MAGIC;
	fcb.f_sectors = fcb_sectors;
	fcb.f_sector_cnt = (uint8_t)sector_cnt;
	erase_oldest = true;

	rc = fcb_init(LOG_HISTORY_AREA, &fcb);

	if (rc) {
		goto out;
	}

	rc = fcb_clear(&fcb);

out:
	__ASSERT_NO_MSG(rc == 0);
}

void log_rpc_history_push(const union log_msg_generic *msg)
{
	int rc;
	size_t len;
	struct fcb_entry entry;

	len = log_msg_generic_get_wlen(&msg->buf) * sizeof(uint32_t);

	k_mutex_lock(&fcb_lock, K_FOREVER);
	rc = fcb_append(&fcb, len, &entry);

	if (rc == -ENOSPC && erase_oldest) {
		/*
		 * The log history FCB is full but overwriting is enabled.
		 * Erase the oldest FCB page and try again.
		 * Note that the "last popped" location is cleared so that the next log transfer
		 * starts from the updated oldest log message.
		 */
		rc = fcb_rotate(&fcb);

		if (rc) {
			goto out;
		}

		memset(&last_popped, 0, sizeof(last_popped));
		rc = fcb_append(&fcb, len, &entry);
	}

	if (rc) {
		goto out;
	}

	rc = flash_area_write(fcb.fap, FCB_ENTRY_FA_DATA_OFF(entry), msg, len);

	if (rc) {
		goto out;
	}

	rc = fcb_append_finish(&fcb, &entry);

out:
	k_mutex_unlock(&fcb_lock);

#ifdef LOG_HISTORY_DEBUG
	__ASSERT_NO_MSG(rc == 0);
#endif
}

void log_rpc_history_set_overwriting(bool overwriting)
{
	k_mutex_lock(&fcb_lock, K_FOREVER);

	erase_oldest = overwriting;

	k_mutex_unlock(&fcb_lock);
}

union log_msg_generic *log_rpc_history_pop(void)
{
	int rc;
	struct fcb_entry entry = last_popped;
	union log_msg_generic *msg = NULL;

	k_mutex_lock(&fcb_lock, K_FOREVER);
	rc = fcb_getnext(&fcb, &entry);

	if (rc) {
		rc = 0;
		goto out;
	}

	msg = (union log_msg_generic *)k_malloc(entry.fe_data_len);

	if (!msg) {
		goto out;
	}

	rc = flash_area_read(fcb.fap, FCB_ENTRY_FA_DATA_OFF(entry), msg, entry.fe_data_len);

	if (rc) {
		goto out;
	}

	last_popped = entry;

	while (fcb.f_oldest != last_popped.fe_sector) {
		rc = fcb_rotate(&fcb);

		if (rc) {
			goto out;
		}
	}

out:
	k_mutex_unlock(&fcb_lock);

#ifdef LOG_HISTORY_DEBUG
	__ASSERT_NO_MSG(rc == 0);
#endif

	return msg;
}

void log_rpc_history_free(const union log_msg_generic *msg)
{
	k_free((void *)msg);
}

uint8_t log_rpc_history_get_usage(void)
{
	int num_free_sectors;

	k_mutex_lock(&fcb_lock, K_FOREVER);

	num_free_sectors = fcb_free_sector_cnt(&fcb);

	k_mutex_unlock(&fcb_lock);

	/*
	 * This calculates the usage with the sector size granularity.
	 * Unforunately, there is no FCB API for getting more accurate usage, but it can be
	 * implemented by hand if the greater accuraccy turns out to be needed.
	 */
	return (fcb.f_sector_cnt - num_free_sectors) * 100 / fcb.f_sector_cnt;
}

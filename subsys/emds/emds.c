/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include "emds_flash.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

static bool emds_ready;
static bool emds_initialized;

static sys_slist_t emds_dynamic_entries;
static struct emds_fs emds_flash;
static emds_store_cb_t app_store_cb;

static int emds_fs_init(void)
{
	int rc;
	const struct flash_area *fa;
	uint32_t sector_cnt = 1;
	struct flash_sector hw_fs;
	size_t emds_sector_size;
	size_t emds_storage_size = 0;
	uint16_t cnt = 0;

	rc = flash_area_open(FIXED_PARTITION_ID(emds_storage), &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_get_sectors(FIXED_PARTITION_ID(emds_storage), &sector_cnt,
				    &hw_fs);
	if (rc == -ENODEV) {
		return rc;
	} else if (rc != 0 && rc != -ENOMEM) {
		k_panic();
	}

	emds_sector_size = hw_fs.fs_size;

	if (emds_sector_size > UINT16_MAX) {
		return -EDOM;
	}

	while (cnt < CONFIG_EMDS_SECTOR_COUNT) {
		emds_storage_size += emds_sector_size;
		if (emds_storage_size > fa->fa_size) {
			break;
		}
		cnt++;
	}

	emds_flash.sector_size = emds_sector_size;
	emds_flash.sector_cnt = cnt;
	emds_flash.offset = fa->fa_off;
	emds_flash.flash_dev = fa->fa_dev;

	return emds_flash_init(&emds_flash);
}


static int emds_entries_size(uint32_t *size)
{
	size_t block_size = emds_flash.flash_params->write_block_size;
	int entries = 0;

	*size = 0;

	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		*size += NRFX_CEIL_DIV(ch->len, block_size) * block_size;
		*size += NRFX_CEIL_DIV(emds_flash.ate_size, block_size) * block_size;
		entries++;
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		*size += NRFX_CEIL_DIV(ch->entry.len, block_size) * block_size;
		*size += NRFX_CEIL_DIV(emds_flash.ate_size, block_size) * block_size;
		entries++;
	}

	return entries;
}

int emds_init(emds_store_cb_t cb)
{
	int rc;

	if (emds_initialized) {
		LOG_DBG("Already initialized");
		return -EALREADY;
	}

	rc = emds_fs_init();
	if (rc) {
		return rc;
	}

	sys_slist_init(&emds_dynamic_entries);

	app_store_cb = cb;

	emds_initialized = true;

	LOG_DBG("Emergency Data Storage initialized.");

	return 0;
}

int emds_entry_add(struct emds_dynamic_entry *entry)
{
	struct emds_dynamic_entry *ch;

	if (!emds_initialized) {
		return -ECANCELED;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		if (ch->entry.id == entry->entry.id) {
			return -EINVAL;
		}
	}

	sys_slist_append(&emds_dynamic_entries, &entry->node);

	emds_ready = false;

	return 0;
}

void store_entries(void)
{
	STRUCT_SECTION_FOREACH(emds_entry, ch)
	{
		ssize_t len = emds_flash_write(&emds_flash, ch->id, ch->data, ch->len);
		if (len < 0) {
			LOG_ERR("Write static entry: (%d) error (%d)", ch->id, len);
		} else if (len != ch->len) {
			LOG_ERR("Write static entry: (%d) failed (%d:%d)", ch->id, ch->len, len);
		}
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		ssize_t len =
			emds_flash_write(&emds_flash, ch->entry.id, ch->entry.data, ch->entry.len);
		if (len < 0) {
			LOG_ERR("Write dynamic entry: (%d) error (%d).", ch->entry.id, len);
		}
		if (len != ch->entry.len) {
			LOG_ERR("Write dynamic entry: (%d) failed (%d:%d).", ch->entry.id,
				ch->entry.len, len);
		}
	}
}

int emds_store(void)
{
	uint32_t store_key;

	if (!emds_ready) {
		return -ECANCELED;
	}

	/* Lock all interrupts */
	store_key = irq_lock();

	/* Start the emergency data storage process. */
	LOG_DBG("Emergency Data Storeage released");

	store_entries();
	emds_ready = false;

	/* Unlock all interrupts */
	irq_unlock(store_key);

	if (app_store_cb) {
		app_store_cb();
	}

	return 0;
}

static void status_reset(void)
{
	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		ch->entry.status.val = EMDS_ENTRY_NO_INIT;
	}

	STRUCT_SECTION_FOREACH(emds_entry, ch)
	{
		*ch->status.ptr = EMDS_ENTRY_NO_INIT;
	}
}

static int entry_load(struct emds_fs *fs, uint16_t id, void *data, size_t entry_len,
		      enum emds_entry_status *status, bool any_entry)
{
	int entry_status;
	ssize_t len = emds_flash_read(fs, id, data, entry_len, &entry_status, any_entry);

	*status = entry_status;
	if (len <= 0 || len != entry_len) {
		LOG_ERR("Failed to retrieve dynamic entry: (%d)", id);
		return -EBADMSG;
	}

	return 0;
}

static int entry_load_itr(bool any_entry)
{
	int err = 0;
	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		if ((ch->entry.status.val != EMDS_ENTRY_VALID) || !any_entry) {
			err |= entry_load(&emds_flash, ch->entry.id, ch->entry.data, ch->entry.len,
					  &ch->entry.status.val, any_entry);
		}
	}

	STRUCT_SECTION_FOREACH(emds_entry, ch)
	{
		if ((*ch->status.ptr != EMDS_ENTRY_VALID) || !any_entry) {
			err |= entry_load(&emds_flash, ch->id, ch->data, ch->len, ch->status.ptr,
					  any_entry);
		}
	}

	return err;
}

int emds_load(void)
{
	status_reset();

	if (emds_flash_is_empty(&emds_flash)) {
		return -ENOMSG;
	}

	if (!emds_initialized) {
		return -ECANCELED;
	}

	int err = entry_load_itr(false);

	if (IS_ENABLED(CONFIG_EMDS_AUTO_RECOVER)) {
		if (err == -EBADMSG) {
			(void)entry_load_itr(true);
			return -EBADMSG;
		}
	}

	return err;
}

int emds_clear(void)
{
	if (!emds_initialized) {
		return -ECANCELED;
	}

	return emds_flash_clear(&emds_flash);
}

int emds_prepare(void)
{
	uint32_t size;
	int rc;

	if (!emds_initialized) {
		return -ECANCELED;
	}

	(void)emds_entries_size(&size);

	rc = emds_flash_prepare(&emds_flash, size);

	/** If flash is empty after prepare, imediatley make an
	 * extra copy of the current entries as backup before the
	 * next EMDS event in case it should fail
	 */
	if (IS_ENABLED(CONFIG_EMDS_AUTO_RECOVER) && rc == 1) {

		ssize_t free_flash = emds_flash_free_space_get(&emds_flash);

		/* Must be enough room for shutdown storage */
		if (free_flash >= size * 2) {
			store_entries();
			rc = emds_flash_prepare(&emds_flash, size);
		}
	}

	if (rc < 0) {
		return rc;
	}

	emds_ready = true;

	return 0;
}

uint32_t emds_store_time_get(void)
{
	size_t block_size = emds_flash.flash_params->write_block_size;
	uint32_t store_time_us = CONFIG_EMDS_FLASH_TIME_BASE_OVERHEAD_US;


	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		store_time_us += NRFX_CEIL_DIV(ch->len, block_size) *
					CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US
			       + NRFX_CEIL_DIV(emds_flash.ate_size, block_size) *
					CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US
			       + CONFIG_EMDS_FLASH_TIME_ENTRY_OVERHEAD_US;
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		store_time_us += NRFX_CEIL_DIV(ch->entry.len, block_size) *
					CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US
			       + NRFX_CEIL_DIV(emds_flash.ate_size, block_size) *
					CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US
			       + CONFIG_EMDS_FLASH_TIME_ENTRY_OVERHEAD_US;
	}

	return store_time_us;
}

uint32_t emds_store_size_get(void)
{
	uint32_t store_size;

	(void)emds_entries_size(&store_size);

	return store_size;
}

bool emds_is_ready(void)
{
	return emds_ready;
}

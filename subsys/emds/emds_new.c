/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include "emds_flash_new.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

static bool emds_ready;
static bool emds_initialized;

static sys_slist_t emds_dynamic_entries;
static struct emds_desc descr;
static emds_store_cb_t app_store_cb;

static void emds_print_init_info(void)
{
	const struct flash_area *fa = descr.fa;
	struct emds_partition_desc *part;

	LOG_DBG("EMDS storage initialized with flash area %s (ID: %d, offset: 0x%lx, size: %zu)",
		fa->fa_dev->name, fa->fa_id, fa->fa_off, fa->fa_size);
	part = &descr.part[0];
	LOG_DBG("Partition 0: size %zu, offset 0x%lx", part->part_size, part->part_off);
	part = &descr.part[1];
	LOG_DBG("Partition 1: size %zu, offset 0x%lx", part->part_size, part->part_off);
	LOG_DBG("Write block size: %zu", emds_get_write_block_size(&descr));
}

static int emds_descr_init(void)
{
	int rc;
	const struct flash_area **fa = &descr.fa;

	rc = flash_area_open(FIXED_PARTITION_ID(emds_storage), fa);
	if (rc) {
		return rc;
	}

	rc = emds_flash_init(&descr);
	if (rc) {
		LOG_ERR("Failed to initialize EMDS flash storage: %d", rc);
		return rc;
	}

	emds_print_init_info();

	return 0;
}

int emds_init(emds_store_cb_t cb)
{
	int rc;

	if (emds_initialized) {
		LOG_DBG("Already initialized");
		return -EALREADY;
	}

	rc = emds_descr_init();
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

static int emds_entries_size(uint32_t *size)
{
	size_t block_size = emds_get_write_block_size(&descr);
	int entries = 0;

	*size = 0;

	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		*size += DIV_ROUND_UP(ch->len + sizeof(struct emds_data_entry), block_size) *
			 block_size;
		*size += DIV_ROUND_UP(sizeof(struct emds_snapshot_metadata), block_size) *
			 block_size;
		entries++;
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		*size += DIV_ROUND_UP(ch->entry.len + sizeof(struct emds_data_entry), block_size) *
			 block_size;
		*size += DIV_ROUND_UP(sizeof(struct emds_snapshot_metadata), block_size) *
			 block_size;
		entries++;
	}

	return entries;
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

uint32_t emds_store_time_get(void)
{
	return 0;
}

int emds_load(void)
{
	if (!emds_initialized) {
		return -ECANCELED;
	}

	return 0;
}

int emds_prepare(void)
{
	emds_ready = true;

	return 0;
}

int emds_store(void)
{
	if (!emds_ready) {
		return -ECANCELED;
	}

	if (app_store_cb) {
		app_store_cb();
	}

	return 0;
}

int emds_clear(void)
{
	if (!emds_initialized) {
		return -ECANCELED;
	}

	return 0;
}

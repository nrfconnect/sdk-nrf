/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include "emds_flash_new.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

#define PARTITIONS_NUM_MAX 2
#define REGIONS_OVERLAP(a, len_a, b, len_b) (((a) < ((b) + (len_b))) && ((b) < ((a) + (len_a))))

static bool emds_ready;
static bool emds_initialized;

static sys_slist_t emds_dynamic_entries;
static struct emds_partition partition[PARTITIONS_NUM_MAX];
static emds_store_cb_t app_store_cb;

static void emds_print_init_info(void)
{
	LOG_DBG("EMDS initialized with the following partitions:");
	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		LOG_DBG("Flash device: %s", partition[i].fa->fa_dev->name);
		LOG_DBG("Partition %d: ID=%d, Offset=0x%08lx, Size=0x%08zx", i,
			partition[i].fa->fa_id, partition[i].fa->fa_off, partition[i].fa->fa_size);
		LOG_DBG("Flash parameters: Write block size=%zu, Erase value=%zu",
			partition[i].fp->write_block_size, partition[i].fp->erase_value);
		LOG_DBG("No explicit erase: %s",
			partition[i].fp->caps.no_explicit_erase ? "true" : "false");
	}
}

static int emds_partition_init(const uint8_t id, struct emds_partition *partition)
{
	int rc;

	rc = flash_area_open(id, &partition->fa);
	if (rc) {
		LOG_ERR("Failed to open EMDS flash storage: %d", rc);
		return rc;
	}

	rc = emds_flash_init(partition);
	if (rc) {
		LOG_ERR("Failed to initialize EMDS flash storage: %d", rc);
		return rc;
	}

	return 0;
}

int emds_init(emds_store_cb_t cb)
{
	const uint8_t id[] = {FIXED_PARTITION_ID(emds_partition_0),
			      FIXED_PARTITION_ID(emds_partition_1)};
	int rc;

	if (emds_initialized) {
		LOG_DBG("Already initialized");
		return -EALREADY;
	}

	if (!FIXED_PARTITION_EXISTS(emds_partition_0) ||
	    !FIXED_PARTITION_EXISTS(emds_partition_1)) {
		LOG_ERR("EMDS partitions not found: %d, %d", id[0], id[1]);
		return -ENODEV;
	}

	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		rc = emds_partition_init(id[i], &partition[i]);
		if (rc) {
			return rc;
		}
	}

	if (REGIONS_OVERLAP(partition[0].fa->fa_off, partition[0].fa->fa_size,
			    partition[1].fa->fa_off, partition[1].fa->fa_size)) {
		LOG_ERR("Flash areas overlap");
		return -EINVAL;
	}

	if (partition[0].fp->write_block_size != partition[1].fp->write_block_size) {
		LOG_ERR("Write block sizes of partitions do not match");
		return -EINVAL;
	}

	emds_print_init_info();

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

static int emds_entries_size(uint32_t *size, size_t alignment)
{
	int entries = 0;

	*size = 0;

	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		*size += ch->len + sizeof(struct emds_data_entry);
		entries++;
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		*size += ch->entry.len + sizeof(struct emds_data_entry);
		entries++;
	}

	*size = DIV_ROUND_UP(*size, alignment) * alignment;
	*size += DIV_ROUND_UP(sizeof(struct emds_snapshot_metadata), alignment) * alignment;

	return entries;
}

uint32_t emds_store_size_get(void)
{
	uint32_t store_size;

	if (!emds_initialized) {
		return -ECANCELED;
	}

	(void)emds_entries_size(&store_size, partition[0].fp->write_block_size);

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

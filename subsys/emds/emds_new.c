/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

#include "emds_flash_new.h"

#include <zephyr/drivers/flash.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

#define PARTITIONS_NUM_MAX 2
#define REGIONS_OVERLAP(a, len_a, b, len_b) (((a) < ((b) + (len_b))) && ((b) < ((a) + (len_a))))

static bool emds_ready;
static bool emds_initialized;
static struct emds_snapshot_candidate freshest_snapshot;

static sys_slist_t emds_dynamic_entries;
static struct emds_partition partition[PARTITIONS_NUM_MAX];
static emds_store_cb_t app_store_cb;

static void emds_print_init_info(void)
{
	LOG_DBG("EMDS initialized with the following partitions:");
	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		LOG_DBG("Flash device: %s", partition[i].fa->fa_dev->name);
		LOG_DBG("Partition %d: ID=%d, Offset=0x%04lx, Size=0x%04zx", i,
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

static uint8_t *emds_entry_memory_get(struct emds_data_entry *entry)
{
	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		if (ch->id == entry->id) {
			entry->length = MIN(ch->len, entry->length);
			return ch->data;
		}
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		if (ch->entry.id == entry->id) {
			entry->length = MIN(ch->entry.len, entry->length);
			return ch->entry.data;
		}
	}

	LOG_WRN("Entry with ID %u not found", entry->id);
	return NULL;
}

static int emds_read_data(const struct flash_area *fa, struct emds_snapshot_metadata *metadata)
{
	struct emds_data_entry entry;
	off_t data_off = metadata->data_instance_off;
	int32_t data_len = metadata->data_instance_len;
	uint8_t *data_buf;
	int rc;

	while (data_len > 0) {
		rc = flash_area_read(fa, data_off, &entry, sizeof(entry));
		if (rc) {
			LOG_ERR("Failed to read data entry: %d", rc);
			return -EIO;
		}

		data_off += sizeof(entry);
		data_len -= sizeof(entry);

		data_buf = emds_entry_memory_get(&entry);

		if (data_buf) {
			rc = flash_area_read(fa, data_off, data_buf, entry.length);
			if (rc) {
				LOG_ERR("Failed to read data for entry ID %u: %d", entry.id, rc);
				return -EIO;
			}
		}

		data_off += entry.length;
		data_len -= entry.length;
	}

	return 0;
}

int emds_load(void)
{
	struct emds_snapshot_candidate candidate = {0};
	int freshest_partition_idx = -1;

	if (!emds_initialized) {
		return -ECANCELED;
	}

	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		if (emds_flash_scan_partition(&partition[i], &candidate)) {
			LOG_ERR("Failed to scan partition: %d", i);
			continue;
		}

		if (freshest_snapshot.metadata.fresh_cnt < candidate.metadata.fresh_cnt) {
			freshest_snapshot = candidate;
			freshest_partition_idx = i;
		}
	}

	if (freshest_partition_idx < 0) {
		LOG_WRN("No valid snapshot found in any partition");
		return -ENOENT;
	}

	LOG_DBG("Found freshest snapshot in partition %d with fresh_cnt %u",
		freshest_partition_idx, freshest_snapshot.metadata.fresh_cnt);

	return emds_read_data(partition[freshest_partition_idx].fa, &freshest_snapshot.metadata);
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
	bool failed = false;
	int rc;

	if (!emds_initialized) {
		return -ECANCELED;
	}

	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		rc = emds_flash_erase_partition(&partition[i]);
		if (rc) {
			LOG_ERR("Failed to erase partition %d error: %d", i, rc);
			failed = true;
		}
	}

	return failed ? -EIO : 0;
}

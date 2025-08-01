/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <emds/emds.h>

#include "emds_flash.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds, CONFIG_EMDS_LOG_LEVEL);

#if NRF52_ERRATA_242_PRESENT
#include <hal/nrf_power.h>
/* Disable POFWARN by writing POFCON before a write or erase operation.
 * Do not attempt to write or erase if EVENTS_POFWARN is already asserted.
 */
static bool pofcon_enabled;

static int suspend_pofwarn(void)
{
	if (!nrf52_errata_242()) {
		return 0;
	}

	bool enabled;
	nrf_power_pof_thr_t pof_thr;

	pof_thr = nrf_power_pofcon_get(NRF_POWER, &enabled);

	if (enabled) {
		nrf_power_pofcon_set(NRF_POWER, false, pof_thr);

		/* This check need to be reworked once POFWARN event will be
		 * served by zephyr.
		 */
		if (nrf_power_event_check(NRF_POWER, NRF_POWER_EVENT_POFWARN)) {
			nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
			return -ECANCELED;
		}

		pofcon_enabled = enabled;
	}

	return 0;
}

static void restore_pofwarn(void)
{
	nrf_power_pof_thr_t pof_thr;

	if (pofcon_enabled) {
		pof_thr = nrf_power_pofcon_get(NRF_POWER, NULL);

		nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
		pofcon_enabled = false;
	}
}

#define SUSPEND_POFWARN() suspend_pofwarn()
#define RESUME_POFWARN()  restore_pofwarn()
#else
#define SUSPEND_POFWARN() 0
#define RESUME_POFWARN()
#endif /* NRF52_ERRATA_242_PRESENT */

#define PARTITIONS_NUM_MAX 2
#define CHUNK_SIZE         16

enum emds_state {
	EMDS_STATE_NOT_INITIALIZED,
	EMDS_STATE_INITIALIZED,
	EMDS_STATE_SYNCHRONIZED,
	EMDS_STATE_READY,
};

static enum emds_state emds_state = EMDS_STATE_NOT_INITIALIZED;
static struct emds_snapshot_candidate freshest_snapshot;
static struct emds_snapshot_candidate allocated_snapshot;

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
		LOG_DBG("%s", flash_params_get_erase_cap(partition[i].fp) & FLASH_ERASE_C_EXPLICIT
				      ? "Explicit erase available"
				      : "Explicit erase not available");
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

	if (emds_state != EMDS_STATE_NOT_INITIALIZED) {
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
	emds_state = EMDS_STATE_INITIALIZED;
	LOG_DBG("Emergency Data Storage initialized.");

	return 0;
}

int emds_entry_add(struct emds_dynamic_entry *entry)
{
	struct emds_dynamic_entry *ch;

	if (emds_state != EMDS_STATE_INITIALIZED) {
		return -ECANCELED;
	}

	STRUCT_SECTION_FOREACH(emds_entry, static_entry) {
		if (static_entry->id == entry->entry.id) {
			return -EINVAL;
		}
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		if (ch->entry.id == entry->entry.id) {
			return -EINVAL;
		}
	}

	sys_slist_append(&emds_dynamic_entries, &entry->node);

	return 0;
}

static int emds_entries_size(size_t *size)
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

	return entries;
}

int emds_store_size_get(size_t *store_size)
{
	if (emds_state == EMDS_STATE_NOT_INITIALIZED) {
		return -ECANCELED;
	}

	(void)emds_entries_size(store_size);

	return 0;
}

bool emds_is_ready(void)
{
	return emds_state == EMDS_STATE_READY;
}

int emds_store_time_get(uint32_t *store_time)
{
	size_t store_size = 0;
	size_t words;
	int chunk_handling;
	int rc;

	rc = emds_store_size_get(&store_size);
	if (rc) {
		return rc;
	}

	words = DIV_ROUND_UP(store_size, 4);
	words += DIV_ROUND_UP(sizeof(struct emds_snapshot_metadata), 4);
	chunk_handling = DIV_ROUND_UP(store_size, CHUNK_SIZE);

	*store_time = words * CONFIG_EMDS_FLASH_TIME_WRITE_ONE_WORD_US;
	*store_time += chunk_handling * CONFIG_EMDS_CHUNK_PREPARATION_TIME_US;

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
	int32_t flash_entry_data_len;
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
		flash_entry_data_len = entry.length;

		data_buf = emds_entry_memory_get(&entry);

		if (data_buf) {
			rc = flash_area_read(fa, data_off, data_buf, entry.length);
			if (rc) {
				LOG_ERR("Failed to read data for entry ID %u: %d", entry.id, rc);
				return -EIO;
			}
		}

		data_off += flash_entry_data_len;
		data_len -= flash_entry_data_len;
	}

	return 0;
}

int emds_load(void)
{
	struct emds_snapshot_candidate candidate = {0};

	if (emds_state == EMDS_STATE_NOT_INITIALIZED) {
		return -ECANCELED;
	}

	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		if (emds_flash_scan_partition(&partition[i], &candidate)) {
			LOG_ERR("Failed to scan partition: %d", i);
			continue;
		}

		if (freshest_snapshot.metadata.fresh_cnt < candidate.metadata.fresh_cnt) {
			freshest_snapshot = candidate;
			freshest_snapshot.partition_index = i;
		}
	}

	emds_state = EMDS_STATE_SYNCHRONIZED;

	if (freshest_snapshot.metadata.fresh_cnt == 0) {
		LOG_WRN("No valid snapshot found in any partition");
		return -ENOENT;
	}

	LOG_DBG("Found freshest snapshot in partition %d with fresh_cnt %u",
		freshest_snapshot.partition_index, freshest_snapshot.metadata.fresh_cnt);

	return emds_read_data(partition[freshest_snapshot.partition_index].fa,
			      &freshest_snapshot.metadata);
}

int emds_prepare(void)
{
	size_t data_size;
	bool erase_enabled = false;
	int idx = 0;
	int freshest_partition_idx = -1;
	int rc = 0;

	if (emds_state != EMDS_STATE_SYNCHRONIZED) {
		return -ECANCELED;
	}

	/* Returned status is not checked since initialization state is checked above */
	(void)emds_store_size_get(&data_size);

	allocated_snapshot.metadata.fresh_cnt = freshest_snapshot.metadata.fresh_cnt + 1;

	/* First try to allocate snapshot in the same partition where freshest snapshot exists */
	if (freshest_snapshot.metadata.fresh_cnt > 0) {
		freshest_partition_idx = freshest_snapshot.partition_index;
		rc = emds_flash_allocate_snapshot(&partition[freshest_partition_idx],
						  &freshest_snapshot, &allocated_snapshot,
						  data_size);
		if (rc == 0) {
			allocated_snapshot.partition_index = freshest_partition_idx;
			emds_state = EMDS_STATE_READY;
			return 0;
		}
		rc = 0;
	}

	do {
		if (idx != freshest_partition_idx) {
			if (erase_enabled) {
				LOG_DBG("Erase partition %d", idx);
				rc = emds_flash_erase_partition(&partition[idx]);
				if (rc) {
					LOG_ERR("Failed to erase partition %d: %d", idx, rc);
					break;
				}
			}

			rc = emds_flash_allocate_snapshot(&partition[idx], NULL,
							  &allocated_snapshot, data_size);
			if (rc == 0) {
				allocated_snapshot.partition_index = idx;
				emds_state = EMDS_STATE_READY;
				return 0;
			}
		}

		idx++;

		if ((idx == PARTITIONS_NUM_MAX) && !erase_enabled && rc == -EADDRINUSE) {
			erase_enabled = true;
			idx = 0;
		}
	} while (idx < PARTITIONS_NUM_MAX);

	return -ENOENT;
}

static void data_stream_pack(uint8_t *in, uint8_t *out, size_t *wp, size_t *rp, size_t len)
{
	size_t size = MIN(CHUNK_SIZE - *wp, len - *rp);

	memcpy(out + *wp, in + *rp, size);
	*rp += size;
	*wp += size;
}

static void data_to_stream(const struct emds_partition *partition, off_t *data_off, uint8_t *in,
			   uint8_t *out, size_t *wp, size_t len)
{
	size_t rp = 0;

	while (rp != len) {
		data_stream_pack(in, out, wp, &rp, len);
		if (*wp == CHUNK_SIZE) {
			allocated_snapshot.metadata.snapshot_crc = crc32_k_4_2_update(
				allocated_snapshot.metadata.snapshot_crc, out, *wp);
			emds_flash_write_data(partition, *data_off, out, *wp);
			*data_off += *wp;
			*wp = 0;
		}
	}
}

static void entry_to_stream(const struct emds_partition *partition, off_t *data_off, uint8_t *out,
			    size_t *wp, struct emds_entry *entry)
{
	struct emds_data_entry data_entry = {
		.id = entry->id,
		.length = entry->len,
	};

	LOG_DBG("Storing entry ID %u, length %u", entry->id, entry->len);
	data_to_stream(partition, data_off, (uint8_t *)&data_entry, out, wp, sizeof(data_entry));
	data_to_stream(partition, data_off, entry->data, out, wp, entry->len);
}

static void stream_fflush(const struct emds_partition *partition, off_t *data_off, uint8_t *out,
			  size_t *wp)
{
	if (*wp > 0) {
		allocated_snapshot.metadata.snapshot_crc =
			crc32_k_4_2_update(allocated_snapshot.metadata.snapshot_crc, out, *wp);
		emds_flash_write_data(partition, *data_off, out, *wp);
		*data_off += *wp;
		*wp = 0;
	}
}

int emds_store(void)
{
	uint32_t store_key;
	uint8_t data_chunk[CHUNK_SIZE];
	size_t wp = 0;
	off_t data_off = allocated_snapshot.metadata.data_instance_off;
	int idx = allocated_snapshot.partition_index;
	int rc = 0;

	if (emds_state != EMDS_STATE_READY) {
		return -ECANCELED;
	}

	/* Lock all interrupts */
	store_key = irq_lock();

	if (SUSPEND_POFWARN()) {
		rc = -ECANCELED;
		goto unlock_and_exit;
	}

	if (flash_params_get_erase_cap(partition[idx].fp) & FLASH_ERASE_C_EXPLICIT) {
		LOG_DBG("Writing metadata on offset: 0x%4lx, address : 0x%4lx",
			 allocated_snapshot.metadata_off,
			 allocated_snapshot.metadata_off + partition[idx].fa->fa_off);
		emds_flash_write_data(&partition[idx], allocated_snapshot.metadata_off,
				      &allocated_snapshot.metadata,
				      offsetof(struct emds_snapshot_metadata, snapshot_crc));
	}

	STRUCT_SECTION_FOREACH(emds_entry, ch) {
		entry_to_stream(&partition[idx], &data_off, data_chunk, &wp, ch);
	}

	struct emds_dynamic_entry *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&emds_dynamic_entries, ch, node) {
		entry_to_stream(&partition[idx], &data_off, data_chunk, &wp, &ch->entry);
	}

	stream_fflush(&partition[idx], &data_off, data_chunk, &wp);

	if (flash_params_get_erase_cap(partition[idx].fp) & FLASH_ERASE_C_EXPLICIT) {
		LOG_DBG("Writing snapshot crc on offset: 0x%4lx, crc : 0x%4x",
			 allocated_snapshot.metadata_off +
					      offsetof(struct emds_snapshot_metadata, snapshot_crc),
			 allocated_snapshot.metadata.snapshot_crc);
		emds_flash_write_data(&partition[idx],
				      allocated_snapshot.metadata_off +
					      offsetof(struct emds_snapshot_metadata, snapshot_crc),
				      &allocated_snapshot.metadata.snapshot_crc, sizeof(uint32_t));
	} else {
		LOG_DBG("Writing metadata on offset: 0x%4lx, address : 0x%4lx, crc : 0x%4x",
			 allocated_snapshot.metadata_off,
			 allocated_snapshot.metadata_off + partition[idx].fa->fa_off,
			 allocated_snapshot.metadata.snapshot_crc);
		emds_flash_write_data(&partition[idx], allocated_snapshot.metadata_off,
				      &allocated_snapshot.metadata,
				      offsetof(struct emds_snapshot_metadata, reserved));
	}

unlock_and_exit:
	emds_state = EMDS_STATE_INITIALIZED;
	RESUME_POFWARN();
	/* Unlock all interrupts */
	irq_unlock(store_key);

	if (app_store_cb && rc == 0) {
		app_store_cb();
	}

	return rc;
}

int emds_clear(void)
{
	bool failed = false;
	int rc;

	if (emds_state == EMDS_STATE_NOT_INITIALIZED) {
		return -ECANCELED;
	}

	emds_state = EMDS_STATE_INITIALIZED;
	memset(&freshest_snapshot, 0, sizeof(freshest_snapshot));
	memset(&allocated_snapshot, 0, sizeof(allocated_snapshot));
	for (int i = 0; i < PARTITIONS_NUM_MAX; i++) {
		rc = emds_flash_erase_partition(&partition[i]);
		if (rc) {
			LOG_ERR("Failed to erase partition %d error: %d", i, rc);
			failed = true;
		}
	}

	return failed ? -EIO : 0;
}

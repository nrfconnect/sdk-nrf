/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "emds_flash_new.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds_flash, CONFIG_EMDS_LOG_LEVEL);

/* "EMDS" in ASCII */
#define EMDS_SNAPSHOT_METADATA_MARKER 0x4D444553
#define CHUNK_SIZE(FP) ((FP)->write_block_size > 16 ? (FP)->write_block_size : 16)

static void cand_list_init(sys_slist_t *cand_list, struct emds_snapshot_candidate *cand_buf)
{
	sys_slist_init(cand_list);
	for (int i = 0; i < CONFIG_EMDS_MAX_CANDIDATES; i++) {
		sys_slist_append(cand_list, &cand_buf[i].node);
	}
}

static void cand_put(sys_slist_t *cand_list, off_t cand_addr, struct emds_snapshot_metadata *cand)
{
	sys_snode_t *cand_node = sys_slist_peek_tail(cand_list);
	sys_snode_t *head = sys_slist_peek_head(cand_list);
	sys_snode_t *curr = head;
	sys_snode_t *prev = curr;
	struct emds_snapshot_candidate *curr_ctx;
	struct emds_snapshot_candidate *placeholder =
		CONTAINER_OF(cand_node, struct emds_snapshot_candidate, node);

	if (cand->fresh_cnt < placeholder->metadata.fresh_cnt) {
		return; /* Ignore candidates that are not fresher than the placeholder */
	}

	LOG_DBG("Found snapshot at offset 0x%04lx with fresh_cnt %u. Metadata address: 0x%04lx",
		cand->data_instance_off, cand->fresh_cnt, cand_addr);

	sys_slist_find_and_remove(cand_list, cand_node);
	placeholder->metadata = *cand;
	placeholder->metadata_addr = cand_addr;

	do {
		curr_ctx = CONTAINER_OF(curr, struct emds_snapshot_candidate, node);
		if (placeholder->metadata.fresh_cnt > curr_ctx->metadata.fresh_cnt) {
			if (curr == head) {
				sys_slist_prepend(cand_list, cand_node);
			} else {
				sys_slist_insert(cand_list, prev, cand_node);
			}
			return;
		}
		prev = curr;
	} while ((curr = sys_slist_peek_next(curr)));

	sys_slist_append(cand_list, cand_node);
}

static bool cand_snapshot_crc_check(const struct emds_partition *partition,
				    struct emds_snapshot_metadata *metadata)
{
	const struct flash_area *fa = partition->fa;
	uint8_t data_chunk[16];
	size_t chunk_size;
	uint32_t crc = 0;
	size_t data_length = metadata->data_instance_len;
	off_t data_off = metadata->data_instance_off;
	int rc;

	while (data_length > 0) {
		chunk_size = MIN(data_length, sizeof(data_chunk));
		rc = flash_area_read(fa, data_off, data_chunk, chunk_size);
		if (rc) {
			LOG_ERR("Failed to read data chunk: %d", rc);
			return false;
		}

		crc = crc32_k_4_2_update(crc, data_chunk, chunk_size);
		data_off += chunk_size;
		data_length -= chunk_size;
	}

	return crc == metadata->snapshot_crc;
}

static bool metadata_iterator(off_t *read_off, int cur_failures)
{
	*read_off -= sizeof(struct emds_snapshot_metadata);
	if (*read_off <= 0) {
		return false;
	}

	return cur_failures <= CONFIG_EMDS_SCANNING_FAILURES;
}

int emds_flash_init(struct emds_partition *partition)
{
	const struct flash_area *fa = partition->fa;

	if (!fa->fa_dev) {
		LOG_ERR("No valid flash device found");
		return -ENXIO;
	}

	partition->fp = flash_get_parameters(fa->fa_dev);
	if (partition->fp == NULL) {
		LOG_ERR("Could not obtain flash parameters");
		return -EINVAL;
	}

	if (!partition->fp->caps.no_explicit_erase) {
		struct flash_pages_info info;
		int rc;

		rc = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &info);
		if (rc) {
			LOG_ERR("Unable to get page info");
			return -EINVAL;
		}

		if (fa->fa_off != info.start_offset || fa->fa_size % info.size) {
			LOG_ERR("Flash area offset or size is not aligned to page size");
			return -EINVAL;
		}
	} else {
		if (fa->fa_off % partition->fp->write_block_size ||
		    fa->fa_size % partition->fp->write_block_size) {
			LOG_ERR("Flash area offset or size is not aligned to write block size");
			return -EINVAL;
		}
	}

	return 0;
}

int emds_flash_scan_partition(const struct emds_partition *partition,
			      struct emds_snapshot_candidate *candidate)
{
	struct emds_snapshot_metadata cache = {0};
	struct emds_snapshot_candidate cand_buf[CONFIG_EMDS_MAX_CANDIDATES] = {0};
	sys_slist_t cand_list;
	sys_snode_t *cand_node;
	const struct flash_area *fa = partition->fa;
	off_t read_off = fa->fa_size - sizeof(cache);
	int failures = 0;
	uint32_t crc;
	int rc;

	cand_list_init(&cand_list, cand_buf);

	do {
		rc = flash_area_read(fa, read_off, &cache, sizeof(cache));
		if (rc) {
			LOG_ERR("Failed to read snapshot metadata: %d", rc);
			return rc;
		}

		if (cache.marker != EMDS_SNAPSHOT_METADATA_MARKER) {
			failures++;
			LOG_DBG("Snapshot metadata marker mismatch at address 0x%04lx",
				fa->fa_off + read_off);
			continue;
		}

		crc = crc32_k_4_2_update(0, (const unsigned char *)&cache,
					 offsetof(struct emds_snapshot_metadata, metadata_crc));
		if (crc != cache.metadata_crc) {
			failures++;
			LOG_DBG("Snapshot metadata CRC mismatch at address 0x%04lx",
				fa->fa_off + read_off);
			continue;
		}

		cand_put(&cand_list, fa->fa_off + read_off, &cache);
	} while (metadata_iterator(&read_off, failures));

	/* At this point we have the sorted linked list of candidates.
	 * Candidates have been sorted from the freshest at the head to the oldest snapshot.
	 */
	while ((cand_node = sys_slist_get(&cand_list))) {
		struct emds_snapshot_candidate *cand =
			CONTAINER_OF(cand_node, struct emds_snapshot_candidate, node);

		if (cand->metadata.fresh_cnt == 0) {
			break;
		}

		if (cand_snapshot_crc_check(partition, &cand->metadata)) {
			*candidate = *cand;
			LOG_DBG("Found valid snapshot at address 0x%04lx with length %u with "
				"fresh_cnt %u",
				fa->fa_off + cand->metadata.data_instance_off,
				cand->metadata.data_instance_len, cand->metadata.fresh_cnt);
			break;
		}
	}

	return 0;
}

int emds_flash_erase_partition(const struct emds_partition *partition)
{
	const struct flash_area *fa = partition->fa;
	int rc;

	rc = flash_area_erase(fa, 0, fa->fa_size);
	if (rc) {
		return rc;
	}

	return 0;
}

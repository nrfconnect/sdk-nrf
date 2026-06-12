/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "mfg_key_blob.h"

#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(mfg_key_blob, LOG_LEVEL_INF);

#define MFG_KEYS_PARTITION mfg_keys_partition

BUILD_ASSERT(PARTITION_EXISTS(MFG_KEYS_PARTITION),
	     "mfg_keys_partition is not defined; add the boards overlay.");

BUILD_ASSERT(sizeof(struct mfg_key_blob) <=
	     PARTITION_SIZE(MFG_KEYS_PARTITION),
	     "mfg_key_blob does not fit in mfg_keys_partition.");

static struct mfg_key_blob blob_cache;
static const struct mfg_key_blob *blob_ptr;
static bool blob_loaded;

static const struct mfg_key_blob *load_blob(void)
{
	const struct flash_area *fa;
	int rc = flash_area_open(PARTITION_ID(MFG_KEYS_PARTITION), &fa);

	if (rc != 0) {
		LOG_ERR("flash_area_open failed: %d", rc);
		return NULL;
	}

	rc = flash_area_read(fa, 0, &blob_cache, sizeof(blob_cache));
	flash_area_close(fa);

	if (rc != 0) {
		LOG_ERR("flash_area_read failed: %d", rc);
		return NULL;
	}

	if (blob_cache.magic != MFG_KEY_BLOB_MAGIC) {
		LOG_ERR("Bad magic 0x%08x (expected 0x%08x)",
			(unsigned int)blob_cache.magic,
			(unsigned int)MFG_KEY_BLOB_MAGIC);
		return NULL;
	}

	if (blob_cache.version != MFG_KEY_BLOB_VERSION) {
		LOG_ERR("Unsupported version %u (expected %u)",
			(unsigned int)blob_cache.version,
			(unsigned int)MFG_KEY_BLOB_VERSION);
		return NULL;
	}

	if (blob_cache.total_size != sizeof(blob_cache)) {
		LOG_ERR("Bad total_size %u (expected %u)",
			(unsigned int)blob_cache.total_size,
			(unsigned int)sizeof(blob_cache));
		return NULL;
	}

	return &blob_cache;
}

const struct mfg_key_blob *mfg_key_blob_get(void)
{
	if (!blob_loaded) {
		blob_ptr = load_blob();
		blob_loaded = true;
	}
	return blob_ptr;
}

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include "emds_flash_new.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds_flash, CONFIG_EMDS_LOG_LEVEL);

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

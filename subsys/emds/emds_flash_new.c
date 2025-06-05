/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/flash.h>
#include "emds_flash_new.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emds_flash, CONFIG_EMDS_LOG_LEVEL);

static int format_partitions_ee(struct emds_desc *desc)
{
	int rc;
	struct flash_pages_info info;
	int available_pages;

	rc = flash_get_page_info_by_offs(desc->fa->fa_dev, desc->fa->fa_off, &info);
	if (rc) {
		LOG_ERR("Unable to get page info");
		return -EINVAL;
	}

	available_pages = (desc->fa->fa_size + desc->fa->fa_off - info.start_offset) / info.size;
	if (available_pages < 2) {
		LOG_ERR("Not enough flash pages available for EMDS storage");
		return -ENOSPC;
	}

	desc->part[0].part_off = info.start_offset;
	desc->part[0].part_size = available_pages / 2 * info.size;
	desc->part[1].part_off = desc->part[0].part_off + desc->part[0].part_size;
	desc->part[1].part_size = desc->part[0].part_size;

	return 0;
}

static int format_partitions_nee(struct emds_desc *desc)
{
	desc->part[0].part_off = desc->fa->fa_off;
	desc->part[0].part_size = desc->fa->fa_size / 2;
	desc->part[1].part_off = desc->fa->fa_off + desc->part[0].part_size;
	desc->part[1].part_size = desc->part[0].part_size;

	return 0;
}

int emds_flash_init(struct emds_desc *desc)
{
	if (!desc->fa->fa_dev) {
		LOG_ERR("No valid flash device found");
		return -ENXIO;
	}

	desc->fp = flash_get_parameters(desc->fa->fa_dev);
	if (desc->fp == NULL) {
		LOG_ERR("Could not obtain flash parameters");
		return -EINVAL;
	}

	if (!!desc->fp->caps.no_explicit_erase) {
		return format_partitions_nee(desc);
	}

	return format_partitions_ee(desc);
}

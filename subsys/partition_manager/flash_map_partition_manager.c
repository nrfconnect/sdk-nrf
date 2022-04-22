/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <storage/flash_map.h>
#include <pm_config.h>
#include <sys/util_macro.h>

#define FLASH_MAP_OFFSET(i) UTIL_CAT(PM_, UTIL_CAT(PM_##i##_LABEL, _OFFSET))
#define FLASH_MAP_DEV(i)    UTIL_CAT(PM_, UTIL_CAT(PM_##i##_LABEL, _DEV_NAME))
#define FLASH_MAP_SIZE(i)   UTIL_CAT(PM_, UTIL_CAT(PM_##i##_LABEL, _SIZE))
#define FLASH_MAP_NUM       PM_NUM

#define FLASH_AREA_FOO(i, _)                \
	{                                       \
		.fa_id       = i,                   \
		.fa_off      = FLASH_MAP_OFFSET(i), \
		.fa_dev_name = FLASH_MAP_DEV(i),    \
		.fa_size     = FLASH_MAP_SIZE(i)    \
	}

const struct flash_area default_flash_map[] = {
	LISTIFY(FLASH_MAP_NUM, FLASH_AREA_FOO, (,))
};

const int flash_map_entries = ARRAY_SIZE(default_flash_map);
const struct flash_area *flash_map = default_flash_map;

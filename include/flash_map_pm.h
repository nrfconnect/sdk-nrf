/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FLASH_MAP_PM_H_
#define FLASH_MAP_PM_H_

#include <pm_config.h>
#include <sys/util.h>

/* Aliases for zephyr - mcuboot/ncs style naming */
#define image_0 mcuboot_primary
#define image_1 mcuboot_secondary

#if (CONFIG_SETTINGS_FCB || CONFIG_SETTINGS_NVS)
#define storage settings_storage
#elif CONFIG_FILE_SYSTEM_LITTLEFS
#define storage littlefs_storage
#endif

#if (CONFIG_SETTINGS_FCB || CONFIG_SETTINGS_NVS) && CONFIG_FILE_SYSTEM_LITTLEFS
#error "Not supported"
#endif

#define PM_ID(label) PM_##label##_ID

#define FLASH_AREA_ID(label) PM_ID(label)

#define FLASH_AREA_OFFSET(label) \
	UTIL_CAT(PM_, UTIL_CAT(UTIL_CAT(PM_, UTIL_CAT(PM_ID(label), _LABEL)), _ADDRESS))

#define FLASH_AREA_SIZE(label) \
	UTIL_CAT(PM_, UTIL_CAT(UTIL_CAT(PM_, UTIL_CAT(PM_ID(label), _LABEL)), _SIZE))

#define FLASH_AREA_LABEL_EXISTS(label) \
	IS_ENABLED(FLASH_AREA_SIZE(label))

#endif /* FLASH_MAP_PM_H_*/

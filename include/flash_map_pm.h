/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FLASH_MAP_PM_H_
#define FLASH_MAP_PM_H_

#include <pm_config.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>

/* Aliases for zephyr - mcuboot/ncs style naming */
#define image_0 mcuboot_primary
#define image_1 mcuboot_secondary
#define image_0_nonsecure mcuboot_primary
#define image_1_nonsecure mcuboot_secondary
#define image_2 mcuboot_primary_1
#define image_3 mcuboot_secondary_1
#define image_scratch mcuboot_scratch

#if (CONFIG_SETTINGS_FCB || CONFIG_SETTINGS_NVS || defined(PM_SETTINGS_STORAGE_ID))
#define storage settings_storage
#elif CONFIG_FILE_SYSTEM_LITTLEFS
#define storage littlefs_storage
#elif CONFIG_NVS
#define storage nvs_storage
#endif

#define PM_ID(label) PM_##label##_ID
#define PM_IS_ENABLED(label) PM_##label##_IS_ENABLED

#define FLASH_AREA_LABEL_STR(label) #label

#define FLASH_AREA_ID(label) PM_ID(label)

#define FLASH_DATA_FIELD(label, x) \
	UTIL_CAT(PM_, UTIL_CAT(UTIL_CAT(PM_, UTIL_CAT(PM_ID(label), _LABEL)), x))

#define FLASH_AREA_OFFSET(label) FLASH_DATA_FIELD(label, _OFFSET)

#define FLASH_AREA_SIZE(label) FLASH_DATA_FIELD(label, _SIZE)

#define FLASH_AREA_DEVICE(label) COND_CODE_1(DT_NODE_EXISTS(FLASH_DATA_FIELD(label, _DEV)), \
				 (DEVICE_DT_GET_OR_NULL(FLASH_DATA_FIELD(label, _DEV))), \
				 (DEVICE_DT_GET_OR_NULL(DT_NODELABEL(FLASH_DATA_FIELD(label, _DEV)))))

#define FLASH_AREA_LABEL_EXISTS(label) \
	IS_ENABLED(PM_IS_ENABLED(label))

#endif /* FLASH_MAP_PM_H_*/

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
#define slot0_partition mcuboot_primary
#define image_1 mcuboot_secondary
#define slot1_partition mcuboot_secondary
#define image_0_nonsecure mcuboot_primary
#define slot0_ns_partition mcuboot_primary
#define image_1_nonsecure mcuboot_secondary
#define slot1_ns_partition mcuboot_secondary
#define image_2 mcuboot_primary_1
#define image_3 mcuboot_secondary_1
#define slot2_partition mcuboot_primary_1
#define slot3_partition mcuboot_secondary_1
#define slot4_partition mcuboot_primary_2
#define slot5_partition mcuboot_secondary_2
#define image_scratch mcuboot_scratch
#define image_scratch mcuboot_scratch

#if (CONFIG_SETTINGS_FCB || CONFIG_SETTINGS_NVS || defined(PM_SETTINGS_STORAGE_ID))
#define storage settings_storage
#define storage_partition settings_storage
#elif CONFIG_FILE_SYSTEM_LITTLEFS
#define storage littlefs_storage
#define storage_partition littlefs_storage
#elif CONFIG_NVS
#define storage nvs_storage
#define storage_partition nvs_storage
#endif

#define PM_ID(label) PM_##label##_ID
#define PM_IS_ENABLED(label) PM_##label##_IS_ENABLED

#define FLASH_AREA_LABEL_STR(label) #label

#define FIXED_PARTITION_ID(label) PM_ID(label)
#define FLASH_AREA_ID(label) FIXED_PARTITION_ID(label)

#define FIXED_PARTITION_DATA_FIELD(label, x) \
	UTIL_CAT(PM_, UTIL_CAT(UTIL_CAT(PM_, UTIL_CAT(PM_ID(label), _LABEL)), x))
#define FLASH_DATA_FIELD(label, x) FIXED_PARTITION_DATA_FIELD(label, x)

#define FIXED_PARTITION_OFFSET(label) FIXED_PARTITION_DATA_FIELD(label, _OFFSET)
#define FLASH_AREA_OFFSET(label) FIXED_PARTITION_OFFSET(label)

#define FIXED_PARTITION_SIZE(label) FIXED_PARTITION_DATA_FIELD(label, _SIZE)
#define FLASH_AREA_SIZE(label) FIXED_PARTITION_SIZE(label)

#define FIXED_PARTITION_DEVICE(label) \
	COND_CODE_1(DT_NODE_EXISTS(FIXED_PARTITION_DATA_FIELD(label, _DEV)), \
		    (DEVICE_DT_GET_OR_NULL(FIXED_PARTITION_DATA_FIELD(label, _DEV))), \
		    (DEVICE_DT_GET_OR_NULL(DT_NODELABEL(FIXED_PARTITION_DATA_FIELD(label, _DEV)))))
#define FLASH_AREA_DEVICE(label) FIXED_PARTITION_DEVICE(label)

#define FIXED_PARTITION_EXISTS(label) IS_ENABLED(PM_IS_ENABLED(label))
#define FLASH_AREA_LABEL_EXISTS(label) FIXED_PARTITION_EXISTS(label)

#endif /* FLASH_MAP_PM_H_*/

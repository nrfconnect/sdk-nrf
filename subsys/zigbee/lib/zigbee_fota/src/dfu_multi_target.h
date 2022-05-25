/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_MULTI_TARGET_H__
#define DFU_MULTI_TARGET_H__

#include <zephyr/kernel.h>

#ifdef CONFIG_ZIGBEE_FOTA_DATA_BLOCK_SIZE
#define DFU_MULTI_TARGET_BUFFER_SIZE CONFIG_ZIGBEE_FOTA_DATA_BLOCK_SIZE
#endif

/**@brief Return the 32-bit incremental identifier of the currently running images. */
uint32_t dfu_multi_target_get_version(void);

/**@brief Initialize the multi-image library with default dfu_target writers. */
int dfu_multi_target_init_default(void);

/**@brief Schedule update of the transferred images on the next boot. */
int dfu_multi_target_schedule_update(void);

#endif /* DFU_MULTI_TARGET_H__ */

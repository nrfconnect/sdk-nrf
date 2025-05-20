/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ORCHESTRATOR_APP_TESTS_COMMON_H__
#define ORCHESTRATOR_APP_TESTS_COMMON_H__

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include <sdfw/sdfw_services/suit_service.h>
#include <suit_plat_mem_util.h>

#define FIXED_PARTITION_WRITE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), write_block_size)

#define DFU_PARTITION_LABEL	 dfu_partition
#define DFU_PARTITION_OFFSET	 FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_ADDRESS	 suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_SIZE	 FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WRITE_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE	 FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

void fill_dfu_partition_with_data(const uint8_t *data_address, size_t data_size);

void ssf_fakes_reset(void);

DECLARE_FAKE_VALUE_FUNC(int, suit_trigger_update, suit_plat_mreg_t *, size_t);

#endif /* ORCHESTRATOR_APP_TESTS_COMMON_H__ */

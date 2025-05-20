/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_TARGET_SUIT_TEST_COMMON_H__
#define DFU_TARGET_SUIT_TEST_COMMON_H__

#include <stdint.h>
#include <stddef.h>

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/fff.h>

#include <sdfw/sdfw_services/suit_service.h>
#include <suit_plat_mem_util.h>

/**
 * @brief Reset all mocked functions.
 *
 */
void reset_fakes(void);

/**
 * @brief Check whether the partition is empty.
 *
 * @param address Partition address to check.
 * @param size Partition size.
 *
 * @retval true If the partition is empty.
 * @retval false If the partition is not empty.
 */
bool is_partition_empty(void *address, size_t size);

DECLARE_FAKE_VALUE_FUNC(int, suit_trigger_update, suit_plat_mreg_t *, size_t);

#endif /* DFU_TARGET_SUIT_TEST_COMMON_H__ */

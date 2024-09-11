/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "common.h"

/* Mocks - ssf services have to be mocked, as real communication with secure domain
 * cannot take place in these tests because native_posix target does not have the secure domain
 * to communicate with, and nRF54H should not reset the device after calling
 * the dfu_target_suit_schedule_update function.
 */
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, suit_trigger_update, suit_plat_mreg_t *, size_t);

void reset_fakes(void)
{
	RESET_FAKE(suit_trigger_update);
}

bool is_partition_empty(void *address, size_t size)
{
	uint8_t *partition_address = (uint8_t *)address;
	bool partition_empty = true;

	for (size_t i = 0; i < size; i++) {
		if (partition_address[i] != 0xFF) {
			partition_empty = false;
			break;
		}
	}
	return partition_empty;
}

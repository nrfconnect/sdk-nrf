/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "orchestrator_app_tests_common.h"

/* Mocks - ssf services have to be mocked, as real communication with secure domain
 * cannot take place in these tests. This is for two reasons:
 * - native_posix and nrf52 platforms do not have the secure domain to communicate with
 * - on nRF54H calling suit_trigger_update would reset a device, which would stop the tests
 */
DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(int, suit_trigger_update, suit_plat_mreg_t *, size_t);

void fill_dfu_partition_with_data(const uint8_t *data_address, size_t data_size)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;
	static uint8_t write_buf[DFU_PARTITION_WRITE_SIZE];

	zassert_true(data_size < DFU_PARTITION_SIZE, "Envelope size exceeds DFU partition size");

	size_t bytes_remaining = data_size;
	size_t data_offset = 0;
	int err;

	while (bytes_remaining >= DFU_PARTITION_WRITE_SIZE) {
		memcpy(write_buf, &data_address[data_offset], DFU_PARTITION_WRITE_SIZE);

		err = flash_write(fdev, DFU_PARTITION_OFFSET + data_offset, write_buf,
				  DFU_PARTITION_WRITE_SIZE);

		zassert_equal(0, err, "Flash write failed");

		bytes_remaining -= DFU_PARTITION_WRITE_SIZE;
		data_offset += DFU_PARTITION_WRITE_SIZE;
	}

	zassert_true(data_offset + DFU_PARTITION_WRITE_SIZE <= DFU_PARTITION_SIZE,
		     "Envelope aligned to flash write size exceeds dfu_partition");

	memset(write_buf, 0xFF, DFU_PARTITION_WRITE_SIZE);
	memcpy(write_buf, &data_address[data_offset], bytes_remaining);
	err = flash_write(fdev, DFU_PARTITION_OFFSET + data_offset, write_buf,
			  DFU_PARTITION_WRITE_SIZE);
}

void ssf_fakes_reset(void)
{
	RESET_FAKE(suit_trigger_update);
}

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <suit_plat_mem_util.h>

#include <suit.h>
#include <suit_platform.h>
#include <suit_mci.h>
#include <suit_storage.h>

#define DFU_PARTITION_ADDRESS (suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET))
#define DFU_PARTITION_OFFSET  ((size_t)FIXED_PARTITION_OFFSET(dfu_partition))
#define DFU_PARTITION_SIZE    ((size_t)FIXED_PARTITION_SIZE(dfu_partition))

static void *test_suit_setup(void)
{
	int err = suit_storage_init();

	zassert_equal(err, 0, "Unable to initialize storage module");

	err = suit_mci_init();
	zassert_equal(err, 0, "Unable to initialize MCI module");

	err = suit_processor_init();
	zassert_equal(err, 0, "Unable to initialise SUIT processor (err: %d)", err);

	return NULL;
}

ZTEST_SUITE(envelope_decoder, NULL, test_suit_setup, NULL, NULL, NULL);

ZTEST(envelope_decoder, test_get_manifest_authenticated_metadata)
{
	int err = suit_processor_get_manifest_metadata(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE,
						       false, NULL, NULL, NULL, NULL);

	zassert_equal(err, 0, "Reading authenticated envelope failed (err: %d)", err);
}

ZTEST(envelope_decoder, test_get_manifest_unauthenticated_metadata)
{
	int err = suit_processor_get_manifest_metadata(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE,
						       true, NULL, NULL, NULL, NULL);

	zassert_equal(err, 0, "Reading unauthenticated envelope failed (err: %d)", err);
}

ZTEST(envelope_decoder, test_process_sequence_parse)
{
	int err = suit_process_sequence(DFU_PARTITION_ADDRESS, DFU_PARTITION_SIZE, SUIT_SEQ_PARSE);

	zassert_equal(err, 0, "Parsing SUIT envelope failed (err: %d)", err);
}

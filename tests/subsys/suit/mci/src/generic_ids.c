/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
#include <suit_mci.h>
#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */

static void *test_suit_setup(void)
{
	int ret = 0;

#ifdef CONFIG_SUIT_STORAGE
	ret = suit_storage_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize SUIT storage");
#endif /* CONFIG_SUIT_STORAGE */

	ret = suit_mci_init();
	zassert_equal(ret, SUIT_PLAT_SUCCESS, "Unable to initialize MCI module");

	return NULL;
}

ZTEST_SUITE(mci_generic_ids_tests, NULL, test_suit_setup, NULL, NULL, NULL);

ZTEST(mci_generic_ids_tests, test_nordic_vendor_id_value)
{

	suit_uuid_t expected_vid = {{0x76, 0x17, 0xda, 0xa5, 0x71, 0xfd, 0x5a, 0x85, 0x8f, 0x94,
				     0xe2, 0x8d, 0x73, 0x5c, 0xe9, 0xf4}};

	const suit_uuid_t *obtained_vid = NULL;
	mci_err_t rc = SUIT_PLAT_SUCCESS;

	rc = suit_mci_nordic_vendor_id_get(&obtained_vid);
	zassert_equal(rc, SUIT_PLAT_SUCCESS, "suit_mci_nordic_vendor_id_get returned (%d)", rc);
	zassert_not_null(obtained_vid, "obtained_vid points to NULL");
	zassert_mem_equal(obtained_vid->raw, expected_vid.raw, sizeof(((suit_uuid_t *)0)->raw),
			  "unexpected vendor_id");
}

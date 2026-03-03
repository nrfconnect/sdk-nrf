/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include "bt_le_audio_tx.h"
#include "hci_vs_sdc/hci_vs_sdc_fake.h"

DEFINE_FFF_GLOBALS;

static void *suite_setup(void)
{
	DO_FOREACH_HCI_VS_SDC_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();

	return NULL;
}

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	DO_FOREACH_HCI_VS_SDC_FAKE(RESET_FAKE);
	FFF_RESET_HISTORY();
}

ZTEST(bt_le_audio_tx, test_stream_started_requires_initialization)
{
	struct stream_index idx = {
		.lvl1 = 0,
		.lvl2 = 0,
		.lvl3 = 0,
	};

	int ret = bt_le_audio_tx_stream_started(idx);

	zassert_equal(-EACCES, ret,
		      "bt_le_audio_tx_stream_started must return -EACCES before init");
}

ZTEST_SUITE(bt_le_audio_tx, NULL, suite_setup, test_setup, NULL, NULL);

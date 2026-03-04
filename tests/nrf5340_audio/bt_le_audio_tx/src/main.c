/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>

#include "bt_le_audio_tx.h"
#include "hci_vs_sdc/hci_vs_sdc_fake.h"
#include "bt_fakes/bt_fakes.h"
#include "zbus_fakes/zbus_fakes.h"
#include "application/application.h"

DEFINE_FFF_GLOBALS;

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	RESET_FAKE(hci_vs_sdc_iso_read_tx_timestamp);
	RESET_FAKE(bt_bap_ep_get_info);
	bt_bap_ep_get_info_fake.custom_fake = bt_bap_ep_get_info_custom_fake;

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

NET_BUF_POOL_FIXED_DEFINE(dummy_pool, 1, 100, sizeof(struct audio_metadata), NULL);

ZTEST(bt_le_audio_tx, test_basic_tx)
{
	int ret;

	static struct net_buf *dummy_audio_frame;

	dummy_audio_frame = net_buf_alloc(&dummy_pool, K_NO_WAIT);
	struct le_audio_tx_info tx[2];

	tx[0].idx.lvl1 = 0;
	tx[0].idx.lvl2 = 0;
	tx[0].idx.lvl3 = 0;
	tx[0].cap_stream = NULL;
	tx[0].audio_location = 0;
	tx[1].idx.lvl1 = 0;
	tx[1].idx.lvl2 = 0;
	tx[1].idx.lvl3 = 1;
	tx[1].cap_stream = NULL;
	tx[1].audio_location = 0;

	struct audio_metadata *meta = net_buf_user_data(dummy_audio_frame);

	meta->data_len_us = 10000;
	meta->bytes_per_location = 0;
	meta->locations = 0x03;

	ret = bt_le_audio_tx_send(dummy_audio_frame, tx, 2);
	zassert_equal(0, ret, "ret %d", ret);
}

ZTEST_SUITE(bt_le_audio_tx, NULL, NULL, test_setup, NULL, NULL);

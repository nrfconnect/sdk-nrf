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
#include "zbus_common.h"

DEFINE_FFF_GLOBALS;

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	RESET_FAKE(hci_vs_sdc_iso_read_tx_timestamp);
	RESET_FAKE(bt_bap_ep_get_info);
	RESET_FAKE(bt_hci_get_conn_handle);
	RESET_FAKE(bt_cap_stream_send);
	RESET_FAKE(bt_cap_stream_send_ts);
	RESET_FAKE(zbus_chan_pub);
	RESET_FAKE(audio_sync_timer_capture);
	RESET_FAKE(bt_audio_codec_cfg_get_frame_dur);
	RESET_FAKE(bt_audio_codec_cfg_frame_dur_to_frame_dur_us);
	RESET_FAKE(bt_audio_codec_cfg_get_octets_per_frame);

	bt_audio_codec_cfg_get_frame_dur_fake.return_val = 0x01;
	bt_audio_codec_cfg_frame_dur_to_frame_dur_us_fake.return_val = 10000;
	bt_bap_ep_get_info_fake.custom_fake = bt_bap_ep_get_info_custom_fake;
	hci_vs_sdc_iso_read_tx_timestamp_fake.custom_fake =
		hci_vs_sdc_iso_read_tx_timestamp_custom_fake;
	bt_audio_codec_cfg_get_octets_per_frame_fake.return_val = 120;
	bt_bap_ep_get_info_fake.return_val = 0;
	bt_hci_get_conn_handle_fake.return_val = 0;
	bt_cap_stream_send_fake.return_val = 0;
	bt_cap_stream_send_ts_fake.return_val = 0;
	zbus_chan_pub_fake.return_val = 0;
	audio_sync_timer_capture_fake.return_val = 0;

	FFF_RESET_HISTORY();
	bt_le_audio_tx_init();
}

NET_BUF_POOL_FIXED_DEFINE(dummy_pool, 1, 240, sizeof(struct audio_metadata), NULL);

#define TEST_SETUP_2                                                                               \
	struct net_buf *dummy_audio_frame;                                                         \
	dummy_audio_frame = net_buf_alloc(&dummy_pool, K_NO_WAIT);                                 \
	uint8_t dummy_data[240];                                                                   \
	net_buf_add_mem(dummy_audio_frame, dummy_data, sizeof(dummy_data));                        \
	struct le_audio_tx_info tx[2];                                                             \
	TEST_CAP_STREAM(cap_stream_0, BT_AUDIO_DIR_SINK, 10000, 1);                                \
	TEST_CAP_STREAM(cap_stream_1, BT_AUDIO_DIR_SINK, 10000, 1);                                \
	tx[0].idx.lvl1 = 0;                                                                        \
	tx[0].idx.lvl2 = 0;                                                                        \
	tx[0].idx.lvl3 = 0;                                                                        \
	tx[0].cap_stream = &cap_stream_0;                                                          \
	tx[0].audio_location = 0;                                                                  \
	tx[1].idx.lvl1 = 0;                                                                        \
	tx[1].idx.lvl2 = 0;                                                                        \
	tx[1].idx.lvl3 = 1;                                                                        \
	tx[1].cap_stream = &cap_stream_1;                                                          \
	tx[1].audio_location = 0;                                                                  \
	struct audio_metadata *meta = net_buf_user_data(dummy_audio_frame);                        \
	meta->data_len_us = 10000;                                                                 \
	meta->bytes_per_location = 120;                                                            \
	meta->locations = 0x03

ZTEST(bt_le_audio_tx, test_basic_tx)
{
	int ret;

	TEST_SETUP_2;
	struct tx_stats stats;

	audio_sync_timer_capture_fake.return_val = 10000;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000;

	ret = bt_le_audio_tx_send(dummy_audio_frame, tx, 2, &stats);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2, bt_cap_stream_send_fake.call_count);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(bt_le_audio_tx, test_bad_parameters)
{
	int ret;

	TEST_SETUP_2;

	ret = bt_le_audio_tx_send(NULL, tx, 2, NULL);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(dummy_audio_frame, NULL, 2, NULL);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(bt_le_audio_tx, test_wrapping)
{
	int ret;

	TEST_SETUP_2;
	struct tx_stats stats;

	ret = bt_le_audio_tx_send(dummy_audio_frame, tx, 2, &stats);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2, bt_cap_stream_send_fake.call_count);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);

	net_buf_unref(dummy_audio_frame);
}

ZTEST_SUITE(bt_le_audio_tx, NULL, NULL, test_setup, NULL, NULL);

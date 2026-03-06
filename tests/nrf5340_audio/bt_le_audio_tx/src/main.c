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

BT_LE_AUDIO_TX_DEFINE(bt_le_audio_tx);

static void test_setup(void *f)
{
	ARG_UNUSED(f);

	RESET_FAKE(hci_vs_sdc_iso_read_tx_timestamp);
	RESET_FAKE(bt_bap_ep_get_info);
	RESET_FAKE(bt_hci_get_conn_handle);
	RESET_FAKE(bt_cap_stream_send);
	bt_cap_stream_send_fake.custom_fake = bt_cap_stream_send_custom_fake;
	RESET_FAKE(bt_cap_stream_send_ts);
	bt_cap_stream_send_ts_fake.custom_fake = bt_cap_stream_send_ts_custom_fake;
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
	zassert_equal(0, bt_le_audio_tx_init(bt_le_audio_tx));
}

NET_BUF_POOL_FIXED_DEFINE(dummy_pool, 1, 240, sizeof(struct audio_metadata), NULL);

#define TEST_SETUP_2                                                                               \
	struct net_buf *dummy_audio_frame;                                                         \
	dummy_audio_frame = net_buf_alloc(&dummy_pool, K_NO_WAIT);                                 \
	uint8_t dummy_data[240];                                                                   \
	net_buf_add_mem(dummy_audio_frame, dummy_data, sizeof(dummy_data));                        \
	struct le_audio_tx_info tx[2];                                                             \
	TEST_CAP_STREAM(cap_stream_0, BT_AUDIO_DIR_SINK, 10000, 1, 10000);                         \
	TEST_CAP_STREAM(cap_stream_1, BT_AUDIO_DIR_SINK, 10000, 1, 10000);                         \
	cap_stream_0.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	cap_stream_1.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
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
	meta->locations = 0x03;                                                                    \
	static const uint8_t chan_to_send = 2;

/* Copy to know the opaque type TODO: REMOVE */
struct tx_inf_dummy {
	uint16_t iso_conn_handle;
	struct bt_iso_tx_info iso_tx;
	struct bt_iso_tx_info iso_tx_readback;
	atomic_t iso_tx_pool_alloc;
	bool hci_wrn_printed;
};

struct bt_le_audio_tx_ctx {
	struct bt_le_audio_tx_cfg cfg;
	struct tx_inf_dummy tx_info_arr[BT_LE_AUDIO_TX_GROUP_MAX][BT_LE_AUDIO_TX_SUBGROUP_MAX]
				       [BT_LE_AUDIO_TX_STREAMS_MAX];
	uint32_t timestamp_ctlr_esti_us;
	bool flush_next;
	bool timestamp_ctlr_esti_us_valid;
	uint32_t timestamp_ctlr_real_us_last;
	uint32_t timestamp_last_correction;
	uint32_t subequent_rapid_corrections;
	int64_t corr_diff_us;
};

static void internals_verify(int64_t corr_diff_us, bool timestamp_ctlr_esti_valid,
			     uint32_t timestamp_ctlr_esti_us, bool flush_next)
{
	struct bt_le_audio_tx_ctx *ctx = bt_le_audio_tx;

	zassert_equal(ctx->corr_diff_us, corr_diff_us, "corr_diff_us %lld expected %lld",
		      ctx->corr_diff_us, corr_diff_us);
	zassert_equal(ctx->timestamp_ctlr_esti_us_valid, timestamp_ctlr_esti_valid,
		      "timestamp_ctlr_esti_valid %d expected %d", ctx->timestamp_ctlr_esti_us_valid,
		      timestamp_ctlr_esti_valid);
	zassert_equal(ctx->timestamp_ctlr_esti_us, timestamp_ctlr_esti_us,
		      "timestamp_ctlr_esti_us %u expected %u", ctx->timestamp_ctlr_esti_us,
		      timestamp_ctlr_esti_us);
	zassert_equal(ctx->flush_next, flush_next, "flush_next %d expected %d", ctx->flush_next,
		      flush_next);
}

static void call_tx_sent(struct le_audio_tx_info *tx, uint8_t num_streams)
{
	for (int i = 0; i < num_streams; i++) {
		struct stream_index idx = tx[i].idx;

		zassert_equal(0, bt_le_audio_tx_stream_sent(bt_le_audio_tx, idx));
	}
}

ZTEST(bt_le_audio_tx, test_basic_tx)
{
	int ret;

	TEST_SETUP_2;

	audio_sync_timer_capture_fake.return_val = 10000;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000;

	ret = bt_le_audio_tx_send(bt_le_audio_tx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2, bt_cap_stream_send_fake.call_count);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, 2);
	internals_verify(0, true, 10000, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(bt_le_audio_tx, test_bad_parameters)
{
	int ret;

	TEST_SETUP_2;

	ret = bt_le_audio_tx_send(bt_le_audio_tx, NULL, tx, chan_to_send);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(bt_le_audio_tx, dummy_audio_frame, NULL, chan_to_send);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	call_tx_sent(tx, chan_to_send);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(bt_le_audio_tx, test_wrapping)
{
	int ret;

	TEST_SETUP_2;

	audio_sync_timer_capture_fake.return_val = UINT32_MAX - 19500;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 20000;

	/* Data is submitted way too late */

	ret = bt_le_audio_tx_send(bt_le_audio_tx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(UINT32_MAX - 30000, false, UINT32_MAX - 20000, false);

	audio_sync_timer_capture_fake.return_val = UINT32_MAX - 9500;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 10000;

	ret = bt_le_audio_tx_send(bt_le_audio_tx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(4, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(2, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(2, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, UINT32_MAX - 10000, false);

	for (int i = 0; i < 10; i++) {
		TC_PRINT("Iteration %d\n", i);
		audio_sync_timer_capture_fake.return_val = UINT32_MAX - 9500 + (i * 10000);
		hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 10000 + (i * 10000);

		ret = bt_le_audio_tx_send(bt_le_audio_tx, dummy_audio_frame, tx, chan_to_send);
		zassert_equal(0, ret, "ret %d", ret);
		zassert_equal(4, bt_cap_stream_send_fake.call_count, "%d",
			      bt_cap_stream_send_fake.call_count);
		zassert_equal(2 * (i + 1), bt_cap_stream_send_ts_fake.call_count);

		zassert_equal(2, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
		zassert_equal(2 + (i + 1), zbus_chan_pub_fake.call_count);
		call_tx_sent(tx, chan_to_send);
		internals_verify(0, true, UINT32_MAX + (i * 10000), false);
	}

	net_buf_unref(dummy_audio_frame);
}

ZTEST_SUITE(bt_le_audio_tx, NULL, NULL, test_setup, NULL, NULL);

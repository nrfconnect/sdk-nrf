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

BT_LE_AUDIO_TX_DEFINE(audio_tx_ctx);

#define TEST_SETUP_1_STREAM                                                                        \
	NET_BUF_POOL_FIXED_DEFINE(dummy_pool_1, 1U, 120U, sizeof(struct audio_metadata), NULL);    \
	struct net_buf *dummy_audio_frame;                                                         \
	dummy_audio_frame = net_buf_alloc(&dummy_pool_1, K_NO_WAIT);                               \
	uint8_t dummy_data[120];                                                                   \
	net_buf_add_mem(dummy_audio_frame, dummy_data, sizeof(dummy_data));                        \
	struct le_audio_tx_info tx[1];                                                             \
	TEST_CAP_STREAM(cap_stream_0, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	cap_stream_0.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	tx[0].idx.lvl1 = 0U;                                                                       \
	tx[0].idx.lvl2 = 0U;                                                                       \
	tx[0].idx.lvl3 = 0U;                                                                       \
	tx[0].cap_stream = &cap_stream_0;                                                          \
	tx[0].audio_location = 0U;                                                                 \
	struct audio_metadata *meta = net_buf_user_data(dummy_audio_frame);                        \
	meta->data_len_us = 10000U;                                                                \
	meta->bytes_per_location = 120U;                                                           \
	meta->locations = 0x01;                                                                    \
	static const uint8_t chan_to_send = 1U;

#define TEST_SETUP_2_STREAMS                                                                       \
	NET_BUF_POOL_FIXED_DEFINE(dummy_pool_2, 1U, 240U, sizeof(struct audio_metadata), NULL);    \
	struct net_buf *dummy_audio_frame;                                                         \
	dummy_audio_frame = net_buf_alloc(&dummy_pool_2, K_NO_WAIT);                               \
	uint8_t dummy_data[120 + 120];                                                             \
	net_buf_add_mem(dummy_audio_frame, dummy_data, sizeof(dummy_data));                        \
	struct le_audio_tx_info tx[2];                                                             \
	TEST_CAP_STREAM(cap_stream_0, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	TEST_CAP_STREAM(cap_stream_1, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	cap_stream_0.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	cap_stream_1.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	tx[0].idx.lvl1 = 0U;                                                                       \
	tx[0].idx.lvl2 = 0U;                                                                       \
	tx[0].idx.lvl3 = 0U;                                                                       \
	tx[0].cap_stream = &cap_stream_0;                                                          \
	tx[0].audio_location = 0U;                                                                 \
	tx[1].idx.lvl1 = 0U;                                                                       \
	tx[1].idx.lvl2 = 0U;                                                                       \
	tx[1].idx.lvl3 = 1U;                                                                       \
	tx[1].cap_stream = &cap_stream_1;                                                          \
	tx[1].audio_location = 0U;                                                                 \
	struct audio_metadata *meta = net_buf_user_data(dummy_audio_frame);                        \
	meta->data_len_us = 10000U;                                                                \
	meta->bytes_per_location = 120U;                                                           \
	meta->locations = 0x03;                                                                    \
	static const uint8_t chan_to_send = 2U;

#define TEST_SETUP_3_STREAMS                                                                       \
	NET_BUF_POOL_FIXED_DEFINE(dummy_pool_3, 1U, 360U, sizeof(struct audio_metadata), NULL);    \
	struct net_buf *dummy_audio_frame;                                                         \
	dummy_audio_frame = net_buf_alloc(&dummy_pool_3, K_NO_WAIT);                               \
	uint8_t dummy_data[120 + 120 + 120];                                                       \
	net_buf_add_mem(dummy_audio_frame, dummy_data, sizeof(dummy_data));                        \
	struct le_audio_tx_info tx[3];                                                             \
	TEST_CAP_STREAM(cap_stream_0, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	TEST_CAP_STREAM(cap_stream_1, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	TEST_CAP_STREAM(cap_stream_2, BT_AUDIO_DIR_SINK, 10000U, 1U, 10000U);                      \
	cap_stream_0.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	cap_stream_1.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	cap_stream_2.bap_stream.ep->state = BT_BAP_EP_STATE_STREAMING;                             \
	tx[0].idx.lvl1 = 0U;                                                                       \
	tx[0].idx.lvl2 = 0U;                                                                       \
	tx[0].idx.lvl3 = 0U;                                                                       \
	tx[0].cap_stream = &cap_stream_0;                                                          \
	tx[0].audio_location = 0U;                                                                 \
	tx[1].idx.lvl1 = 0U;                                                                       \
	tx[1].idx.lvl2 = 0U;                                                                       \
	tx[1].idx.lvl3 = 1U;                                                                       \
	tx[1].cap_stream = &cap_stream_1;                                                          \
	tx[1].audio_location = 0U;                                                                 \
	tx[2].idx.lvl1 = 0U;                                                                       \
	tx[2].idx.lvl2 = 0U;                                                                       \
	tx[2].idx.lvl3 = 2U;                                                                       \
	tx[2].cap_stream = &cap_stream_2;                                                          \
	tx[2].audio_location = 0U;                                                                 \
	struct audio_metadata *meta = net_buf_user_data(dummy_audio_frame);                        \
	meta->data_len_us = 10000U;                                                                \
	meta->bytes_per_location = 120U;                                                           \
	meta->locations = 0x07;                                                                    \
	static const uint8_t chan_to_send = 3U;

static void internals_verify(int64_t corr_diff_us, bool ts_ctlr_esti_valid,
			     uint32_t ts_ctlr_esti_us, bool flush_next)
{
	struct bt_le_audio_tx_ctx *ctx = (struct bt_le_audio_tx_ctx *)audio_tx_ctx;

	zassert_equal(ctx->corr_diff_us, corr_diff_us, "corr_diff_us %lld expected %lld",
		      ctx->corr_diff_us, corr_diff_us);
	zassert_equal(ctx->ts_ctlr_esti_us_valid, ts_ctlr_esti_valid,
		      "ts_ctlr_esti_valid %d expected %d", ctx->ts_ctlr_esti_us_valid,
		      ts_ctlr_esti_valid);
	zassert_equal(ctx->ts_ctlr_esti_us, ts_ctlr_esti_us, "ts_ctlr_esti_us %u expected %u",
		      ctx->ts_ctlr_esti_us, ts_ctlr_esti_us);
	zassert_equal(ctx->flush_next, flush_next, "flush_next %d expected %d", ctx->flush_next,
		      flush_next);
}

static void call_tx_sent(struct le_audio_tx_info *tx, uint8_t num_streams)
{
	int ret;

	for (int i = 0; i < num_streams; i++) {
		struct stream_index idx = tx[i].idx;

		ret = bt_le_audio_tx_stream_sent(audio_tx_ctx, idx);
		zassert_equal(0, ret, "ret: %d, ret", ret);
	}
}

ZTEST(audio_tx, test_tx_send_basic_1_stream)
{
	/* Verifies the nominal TX path with one active stream. */
	int ret;

	TEST_SETUP_1_STREAM;

	audio_sync_timer_capture_fake.return_val = 10000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(1U, bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, 1U);
	internals_verify(0, true, 10000U, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_basic_2_streams)
{
	/* Verifies the nominal TX path with two active streams. */
	int ret;

	TEST_SETUP_2_STREAMS;

	audio_sync_timer_capture_fake.return_val = 10000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2U, bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, 2U);
	internals_verify(0, true, 10000U, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_basic_3_streams)
{
	/* Verifies the nominal TX path with three active streams. */
	int ret;

	TEST_SETUP_3_STREAMS;

	audio_sync_timer_capture_fake.return_val = 10000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(3U, bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, 3U);
	internals_verify(0, true, 10000U, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_bad_parameters)
{
	int ret;

	TEST_SETUP_2_STREAMS;

	ret = bt_le_audio_tx_send(NULL, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(audio_tx_ctx, NULL, tx, chan_to_send);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, NULL, chan_to_send);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, 5);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, 0);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	call_tx_sent(tx, chan_to_send);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_too_late)
{
	int ret;

	TEST_SETUP_2_STREAMS;

	audio_sync_timer_capture_fake.return_val = UINT32_MAX - 20500U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 20000U;

	/* Data is submitted way too late */
	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(-ETIMEDOUT, ret, "ret %d", ret);
	zassert_equal(2U, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(UINT32_MAX - 30000U, false, UINT32_MAX - 20000U, false);

	audio_sync_timer_capture_fake.return_val = UINT32_MAX - 10500U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(4U, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(2U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(2U, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, UINT32_MAX - 10000U, false);

	audio_sync_timer_capture_fake.return_val = UINT32_MAX - 500U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX - 0U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(4U, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(2U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(2U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(3U, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, UINT32_MAX - 0U, false);

	audio_sync_timer_capture_fake.return_val = UINT32_MAX + 9500U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = UINT32_MAX + 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(4U, bt_cap_stream_send_fake.call_count, "%d",
		      bt_cap_stream_send_fake.call_count);
	zassert_equal(4U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(2U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(4U, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, UINT32_MAX + 10000U, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_many)
{
	/* Verifies ts logic across uint32_t wrap and steady-state sends.
	 * In order to ensure a uint32_t wrap, and given 10000 us between calls:
	 * we need at least (UINT32_MAX / 10000) iterations = 429 497.
	 */
	int ret;

	TEST_SETUP_2_STREAMS;

	const uint32_t NUM_ITER = 430000U;
	uint32_t read_tx_ts_calls = 0U;

	for (uint32_t i = 0U; i < NUM_ITER; i++) {
		if (i % 100U == 0U) {
			read_tx_ts_calls++;
		}

		audio_sync_timer_capture_fake.return_val = i * 10000U;
		hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = (i + 1U) * 10000U;

		ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
		zassert_equal(0, ret, "ret %d", ret);

		if (i == 0U) {
			zassert_equal(0, bt_cap_stream_send_ts_fake.call_count, "%d",
				      bt_cap_stream_send_ts_fake.call_count);
		} else {
			zassert_equal((i * 2U), bt_cap_stream_send_ts_fake.call_count, "%d",
				      bt_cap_stream_send_ts_fake.call_count);
		}

		zassert_equal(2U, bt_cap_stream_send_fake.call_count);

		zassert_equal(read_tx_ts_calls, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
		zassert_equal(i + 1U, zbus_chan_pub_fake.call_count);
		call_tx_sent(tx, chan_to_send);
		internals_verify(0U, true, (i + 1U) * 10000U, false);
	}

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_test_api_call_too_slow)
{
	/* Test what happens when we call the API too slow */
	int ret;

	TEST_SETUP_2_STREAMS;

	/* One standard/uneventful send */
	audio_sync_timer_capture_fake.return_val = 0;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count, "%d",
		      bt_cap_stream_send_ts_fake.call_count);
	zassert_equal(2, bt_cap_stream_send_fake.call_count);
	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, 10000U, false);

	/* Simulate that we called too late */
	audio_sync_timer_capture_fake.return_val = 13000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 20000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);

	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count, "%d",
		      bt_cap_stream_send_ts_fake.call_count);
	zassert_equal(4, bt_cap_stream_send_fake.call_count);
	zassert_equal(2, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(2, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, 20000U, false);
}

ZTEST(audio_tx, test_tx_test_api_call_too_fast)
{
	/* Test what happens when we call the API too fast */
	int ret;

	TEST_SETUP_2_STREAMS;

	/* One standard/uneventful send */
	audio_sync_timer_capture_fake.return_val = 0;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count, "%d",
		      bt_cap_stream_send_ts_fake.call_count);
	zassert_equal(2, bt_cap_stream_send_fake.call_count);
	zassert_equal(1, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, 10000U, false);

	/* Simulate that we called too fast */
	audio_sync_timer_capture_fake.return_val = 7000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 20000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);

	zassert_equal(0, bt_cap_stream_send_ts_fake.call_count, "%d",
		      bt_cap_stream_send_ts_fake.call_count);
	zassert_equal(4, bt_cap_stream_send_fake.call_count);
	zassert_equal(2, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(2, zbus_chan_pub_fake.call_count);
	call_tx_sent(tx, chan_to_send);
	internals_verify(0, true, 20000U, false);
}

/* Test for what happens when the controller returns drifted timestamps*/

ZTEST(audio_tx, test_tx_send_send_too_fast)
{
	/* Verifies early submission (compared to controller time)
	 * handling and flush behavior on next call.
	 */
	int ret;

	TEST_SETUP_2_STREAMS;

	audio_sync_timer_capture_fake.return_val = 10000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 5000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2U, bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, chan_to_send);
	internals_verify(-5000, false, 5000U, true);

	/* Call send again immediately, should trigger flush */
	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(-ECANCELED, ret, "ret %d", ret);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_tx_send_send_uncommon_channels)
{
	/* Verifies the nominal TX path with uncommon channel combination. */
	int ret;

	TEST_SETUP_2_STREAMS;
	meta->locations = BT_AUDIO_LOCATION_TOP_FRONT_LEFT | BT_AUDIO_LOCATION_FRONT_RIGHT_WIDE;

	audio_sync_timer_capture_fake.return_val = 10000U;
	hci_vs_sdc_iso_read_tx_timestamp_fake.return_val = 10000U;

	ret = bt_le_audio_tx_send(audio_tx_ctx, dummy_audio_frame, tx, chan_to_send);
	zassert_equal(0, ret, "ret %d", ret);
	zassert_equal(2U, bt_cap_stream_send_fake.call_count);
	zassert_equal(0U, bt_cap_stream_send_ts_fake.call_count);

	zassert_equal(1U, hci_vs_sdc_iso_read_tx_timestamp_fake.call_count);
	zassert_equal(1U, zbus_chan_pub_fake.call_count);

	call_tx_sent(tx, 2U);
	internals_verify(0, true, 10000U, false);

	net_buf_unref(dummy_audio_frame);
}

ZTEST(audio_tx, test_bad_parameters_tx_init)
{
	/* Verifies argument validation for NULL input pointers. */
	int ret;

	ret = bt_le_audio_tx_init(NULL, true);
	zassert_equal(-EINVAL, ret, "ret %d", ret);
}

ZTEST(audio_tx, test_bad_parameters_stream_started)
{
	int ret;
	struct stream_index idx = {0};

	ret = bt_le_audio_tx_stream_started(NULL, idx);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	idx.lvl1 = 50U;
	idx.lvl2 = 0U;
	idx.lvl3 = 0U;

	ret = bt_le_audio_tx_stream_started(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);

	idx.lvl1 = 0U;
	idx.lvl2 = 50U;
	idx.lvl3 = 0U;

	ret = bt_le_audio_tx_stream_started(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);

	idx.lvl1 = 0U;
	idx.lvl2 = 0U;
	idx.lvl3 = 50U;

	ret = bt_le_audio_tx_stream_started(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);
}

ZTEST(audio_tx, test_bad_parameters_stream_sent)
{
	int ret;
	struct stream_index idx = {0};

	ret = bt_le_audio_tx_stream_sent(NULL, idx);
	zassert_equal(-EINVAL, ret, "ret %d", ret);

	idx.lvl1 = 50U;
	idx.lvl2 = 0U;
	idx.lvl3 = 0U;

	ret = bt_le_audio_tx_stream_sent(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);

	idx.lvl1 = 0U;
	idx.lvl2 = 50U;
	idx.lvl3 = 0U;

	ret = bt_le_audio_tx_stream_sent(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);

	idx.lvl1 = 0U;
	idx.lvl2 = 0U;
	idx.lvl3 = 50U;

	ret = bt_le_audio_tx_stream_sent(audio_tx_ctx, idx);
	zassert_equal(-ESPIPE, ret, "ret %d", ret);
}

static void before_fn(void *f)
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
	/* Initialize the TX context as a Bluetooth central device
	 * At current time, this only impacts print options.
	 */
	zassert_equal(0, bt_le_audio_tx_init(audio_tx_ctx, true));
}

ZTEST_SUITE(audio_tx, NULL, NULL, before_fn, NULL, NULL);
